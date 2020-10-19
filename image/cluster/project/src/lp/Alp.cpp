/*
 * Alp.cpp
 *
 *  Created on: 9 Sep 2010
 *      Author: Dr B.G.W. Craenen <b.craenen@cs.bham.ac.uk>
 */
#include <cassert>
#include "Alp.h"
#include "Agent.h"
#include "Log.h"
#include "Initialisor.h"
#include "spdlog/spdlog.h"

#ifdef TIMER
#include "Helper.h"
#include "FilePrint.h"
#endif

#include "SingleReadAntiMessage.h"
#include "WriteAntiMessage.h"

Alp::Alp(unsigned int pRank, unsigned int pCommSize,
         unsigned int pNumberOfClps, unsigned int pNumberOfAlps,
         unsigned long pStartTime, unsigned long pEndTime,
         const Initialisor *initialisor) {
  SetRank(pRank);
  SetSize(pCommSize);
  SetNumberOfClps(pNumberOfClps);
  SetNumberOfAlps(pNumberOfAlps);
  fStartTime = pStartTime;
  fEndTime = pEndTime;
  SetGvt(fStartTime);

  message_id_handler_ = new IdentifierHandler(pRank, pNumberOfClps, pNumberOfAlps);

  fGVTCalculator = new GvtCalculator(this);
  fGVT = fStartTime;


  fProcessMessageMutex = Mutex(ERRORCHECK);


  auto iter = initialisor->GetAlpToClpMap().find(GetRank());
  if (iter != initialisor->GetAlpToClpMap().end()) {
    fParentClp = iter->second;
  } else {
    spdlog::critical("Alp couldn't init parent rank!");
    exit(1);
  }

  fMPIInterface = new MpiInterface(this, this);

  MPI_Barrier(MPI_COMM_WORLD);

  fMPIInterface->Signal();

  MPI_Barrier(MPI_COMM_WORLD);
}

//Semaphore &Alp::GetWaitingSemaphore(unsigned long agent_id) {
//  auto old_sem = agent_waiting_semaphore_map_.find(agent_id);
//  if (old_sem != agent_waiting_semaphore_map_.end()) {
//    // cleanup old
//    // deleting nullptr is ok
//    delete old_sem->second;
//  }
//  auto new_semaphore = new Semaphore();
//  agent_waiting_semaphore_map_[agent_id] = new_semaphore;
//  return *agent_waiting_semaphore_map_[agent_id];
//}

const AbstractMessage *Alp::GetResponseMessage(unsigned long agent_id) const {
  auto it = agent_response_map_.find(agent_id);

  assert(it != agent_response_map_.end());
  auto v = it->second;
  if (v == nullptr) {
    spdlog::critical("GetResponseMessage nullptr encountered!!! {}, time {}, waiting for message {}", agent_id,
                     this->GetAlpManagedAgentLvt(agent_id),
                     agent_response_message_id_map_.find(agent_id)->second);
    exit(1);
  }

  //agent_response_map_.erase(it);
  return v;
}

bool Alp::AddAgent(Agent *agent) {
  unsigned long agent_id = agent->agent_id();
  if (managed_agents_.find(agent_id) != managed_agents_.end()) {
    return false;// already in list
  }
  managed_agents_[agent_id] = agent;
  agent_lvt_map_[agent_id] = 0;
  agent_lvt_history_map_[agent_id] = list<unsigned long>{0};
  agent_cancel_flag_map_[agent_id] = false;
  agent_rollback_reentry_lock_map_[agent_id] = Mutex(NORMAL);
  agent->attach_alp(this);
  return true;
}

void Alp::Send() {
  AbstractMessage *message;
  if (!fSendControlMessageQueue->IsEmpty()) {
    message = fSendControlMessageQueue->DequeueMessage();
  } else if (!fSendMessageQueue->IsEmpty()) {
    message = fSendMessageQueue->DequeueMessage();
    switch (message->GetType()) {
      case SINGLEREADMESSAGE: {
        SingleReadMessage *singleReadMessage = static_cast<SingleReadMessage *>(message);
        AddToSendList(singleReadMessage);
        PreProcessSendMessage(singleReadMessage);
        agent_response_message_id_map_[singleReadMessage->GetOriginalAgent().GetId()] = singleReadMessage->GetIdentifier();
      }
        break;
      case SINGLEREADRESPONSEMESSAGE :
        PreProcessSendMessage(dynamic_cast<SingleReadResponseMessage *> (message));
        break;
      case SINGLEREADANTIMESSAGE :
        PreProcessSendMessage(static_cast<SingleReadAntiMessage *> (message));
        break;
      case WRITEMESSAGE : {
        WriteMessage *writeMessage = static_cast<WriteMessage *>(message);
        AddToSendList(writeMessage);
        PreProcessSendMessage(writeMessage);
        agent_response_message_id_map_[writeMessage->GetOriginalAgent().GetId()] = writeMessage->GetIdentifier();
      }
        break;
      case WRITERESPONSEMESSAGE :
        PreProcessSendMessage(static_cast<WriteResponseMessage *> (message));
        break;
      case WRITEANTIMESSAGE :
        PreProcessSendMessage(static_cast<WriteAntiMessage *> (message));
        break;
      case RANGEQUERYMESSAGE: {
        RangeQueryMessage *rangeQueryMessage = static_cast<RangeQueryMessage *>(message);
        AddToSendList(rangeQueryMessage);
        PreProcessSendMessage(rangeQueryMessage);
        agent_response_message_id_map_[rangeQueryMessage->GetOriginalAgent().GetId()] = rangeQueryMessage->GetIdentifier();
      }
        break;
      case RANGEQUERYANTIMESSAGE :
        PreProcessSendMessage(static_cast<RangeQueryAntiMessage *> (message));
        break;
      case ROLLBACKMESSAGE :
        PreProcessSendMessage(static_cast<RollbackMessage *> (message));
        break;
      default: // skip
        break;
    }
  } else {
    // A message signal was issues, but no message remains to be send, most probably because
    // the message was removed because of a rollback.
    return;
  }
  ostringstream out;
  message->Serialise(out);
  spdlog::debug("ALP send message: {}", out.str());
  fMPIInterface->Send(message);
}

void Alp::Receive() {
  // Fetch received message from the receive queue
  AbstractMessage *message = fReceiveMessageQueue->DequeueMessage();
  //spdlog::debug("Message arrived, rank {0}, type {1}", this->GetRank(), message->GetType());
  ostringstream out;
  message->Serialise(out);
  spdlog::debug("ALP receive message: {}", out.str());
  fProcessMessageMutex.Lock();
  switch (message->GetType()) {
    case SINGLEREADRESPONSEMESSAGE: {
      SingleReadResponseMessage *singleReadResponseMessage = dynamic_cast<SingleReadResponseMessage *>(message);
      PreProcessReceiveMessage(singleReadResponseMessage);
      ProcessMessage(singleReadResponseMessage);
    }
      break;
    case WRITERESPONSEMESSAGE: {
      WriteResponseMessage *writeResponseMessage = dynamic_cast<WriteResponseMessage *>(message);
      PreProcessReceiveMessage(writeResponseMessage);
      ProcessMessage(writeResponseMessage);
    }
      break;
    case RANGEQUERYMESSAGE: {
      RangeQueryMessage *rangeQueryMessage = dynamic_cast<RangeQueryMessage *>(message);
      PreProcessReceiveMessage(rangeQueryMessage);
      ProcessMessage(rangeQueryMessage);
    }
      break;
    case ROLLBACKMESSAGE: {
      RollbackMessage *rollbackMessage = dynamic_cast<RollbackMessage *>(message);
      PreProcessReceiveMessage(rollbackMessage);
      ProcessMessage(rollbackMessage);
    }
      break;
    case GVTCONTROLMESSAGE:
      fGVTCalculator->ProcessMessage(
          dynamic_cast<const GvtControlMessage *> (message));
      break;
    case GVTREQUESTMESSAGE:
      fGVTCalculator->ProcessMessage(
          dynamic_cast<const GvtRequestMessage *> (message));
      break;
    case GVTVALUEMESSAGE:
      fGVTCalculator->ProcessMessage(
          dynamic_cast<const GvtValueMessage *> (message));
      break;
    default:
      ostringstream out;
      (*message).Serialise(out);
      spdlog::critical("Alp {} received unknown message: {}", GetRank(), out.str());
      exit(1);
  }
  fProcessMessageMutex.Unlock();
  /*
   * Don't free the memory for value here! That memory _address_ is copied for the interface,
   * and the value is still needed there. Value memory will be freed there.
   */
  // ADDED BY Pill: Don't delete it for now. Let agent do it (or simply ignore since it's singleton anyway)
  //delete message;
}

void Alp::ProcessMessage(const RollbackMessage *pRollbackMessage) {

  unsigned long agent_id = pRollbackMessage->GetOriginalAgent().GetId();
  agent_rollback_reentry_lock_map_[agent_id].Lock();
  ProcessRollback(pRollbackMessage);
  agent_rollback_reentry_lock_map_[agent_id].Unlock();

}

void Alp::ProcessMessage(const SingleReadResponseMessage *pSingleReadResponseMessage) {
  //unsigned long message_id = pSingleReadResponseMessage->GetIdentifier();
  unsigned long agent_id = pSingleReadResponseMessage->GetOriginalAgent().GetId();
  //spdlog::debug("Agent {} received SingleReadResponseMessage", agent_id);
  if (agent_response_message_id_map_[agent_id] == pSingleReadResponseMessage->GetIdentifier()) {
    spdlog::debug("Alp::ProcessMessage(SingleReadResponseMessage): Agent {}, get response message id = {}", agent_id,
                  agent_response_message_id_map_[agent_id]);
    agent_response_map_[agent_id] = pSingleReadResponseMessage;
    this->GetAgent(agent_id)->SetMessageArriveFlag();
  }
}

void Alp::ProcessMessage(const WriteResponseMessage *pWriteResponseMessage) {
  //unsigned long message_id = pWriteResponseMessage->GetIdentifier();
  unsigned long agent_id = pWriteResponseMessage->GetOriginalAgent().GetId();
  //spdlog::debug("Agent {} received WriteResponseMessage", agent_id);
  // check if the write is success
  // if not, since the write period is not generated, this message needs rto be removed from send list
  bool is_success = (pWriteResponseMessage->GetWriteStatus() == writeSUCCESS);
  if (!is_success) {
    bool remove_success = RemoveFromSendList(pWriteResponseMessage->GetIdentifier());
    if (!remove_success) {
      spdlog::error("RemoveFromSendList({}) remove failed!", pWriteResponseMessage->GetIdentifier());
    }
  }
  if (agent_response_message_id_map_[agent_id] == pWriteResponseMessage->GetIdentifier()) {
    spdlog::debug("Alp::ProcessMessage(WriteResponseMessage): Agent {}, get response message id = {}", agent_id,
                  agent_response_message_id_map_[agent_id]);
    agent_response_map_[agent_id] = pWriteResponseMessage;
    this->GetAgent(agent_id)->SetMessageArriveFlag();
  } else {
    spdlog::debug("Alp::ProcessMessage(WriteResponseMessage): Message discarded, id {}, agent {} waiting for {}",
                  pWriteResponseMessage->GetIdentifier(), agent_id, agent_response_message_id_map_[agent_id]);
  }
}

void Alp::ProcessMessage(const RangeQueryMessage *pRangeQueryMessage) {
  //unsigned long message_id = pRangeQueryMessage->GetIdentifier();
  unsigned long agent_id = pRangeQueryMessage->GetOriginalAgent().GetId();
  //spdlog::debug("Agent {} received RangeQueryMessage", agent_id);
  if (agent_response_message_id_map_[agent_id] == pRangeQueryMessage->GetIdentifier()) {
    //spdlog::debug("Signal agent {}, response message {}", agent_id, agent_response_message_id_map_[agent_id]);
    agent_response_map_[agent_id] = pRangeQueryMessage;
    this->GetAgent(agent_id)->SetMessageArriveFlag();
  }


}

/*When rollback message arrives, agent can be in the waiting state or not in waiting state
 * If not waiting, just rollback the statebase and LVT
 * If waiting, need to signal that semaphore, and perform statebase rollback and LVT rollback,
 * and discard incoming invalidated message (if any)
 * Also, the agent thread needs to be suspended
 */
bool Alp::ProcessRollback(const RollbackMessage *pRollbackMessage) {
  // commonly used


  unsigned long rollback_message_timestamp = pRollbackMessage->GetTimestamp();
  unsigned long agent_id = pRollbackMessage->GetOriginalAgent().GetId();
  Agent *agent = this->GetAgent(agent_id);
  spdlog::debug("Alp::ProcessRollback Agent {0} rollback message received, timestamp {1}, current GVT {2}",
                agent_id, rollback_message_timestamp, this->GetGvt());
  if (rollback_message_timestamp < this->GetGvt()) {
    spdlog::critical(
        "Alp::ProcessRollback rollback message timestamp smaller than GVT! Agent {0}, timestamp {1}, current GVT {2}",
        agent_id, rollback_message_timestamp, this->GetGvt());
    exit(1);
  }
  // Check if rollback already on tag list
  if (CheckRollbackTagList(pRollbackMessage->GetRollbackTag())) {
    spdlog::debug("Alp::ProcessRollback: RB tag already in list, ignored");
    return false;
  }
  // Check if rollback rolls back far enough
  if (rollback_message_timestamp >= agent->GetLVT()) {
    spdlog::warn(
        "Alp::ProcessRollback(): Rollback time not smaller than LVT, agent: {}, local LVT {}, rollback time: {}",
        agent_id, agent->GetLVT(), rollback_message_timestamp);
    return false;
  }
  // Rollback message is good, rollback

  // Reset LVT map

  if (rollback_message_timestamp < 0) {
    spdlog::critical("Alp::ProcessRollback(): Rollback time smaller than 0, agent: {}, LVT {}, rollback time: {}",
                     agent_id, GetAlpManagedAgentLvt(agent_id), rollback_message_timestamp);
    exit(1);
  }

  //spdlog::debug("Process rollback!");


  // stop the agent
  spdlog::debug("Agent {} RB stopping", agent_id);
  agent->Abort();
  spdlog::debug("Agent {} agent->Abort()", agent_id);

  // should have been stopped if not in final waiting stage
  // cancel flag need to be set before resetting semaphore
  SetCancelFlag(agent_id, true);
  spdlog::debug("Agent {} SetCancelFlag", agent_id);

  // should have been stopped if in final waiting stage
  agent->ResetMessageArriveFlag();
  spdlog::debug("Agent {} ResetMessageArriveFlag", agent_id);

  //delete agent_response_map_[agent_id];

  agent->Join();
  spdlog::debug("Agent {} RB stopped", agent_id);
  //agent_response_map_[agent_id] = nullptr;
  //spdlog::debug("agent_response_map_[{}] set to nullptr", agent_id);

  //agent_response_message_id_map_[agent_id] = 0;

  // rollback LVT and history

  unsigned long rollback_to_timestamp = this->RollbackAgentLvtHistory(agent_id, rollback_message_timestamp);
  if (rollback_to_timestamp < this->GetGvt()) {
    spdlog::critical(
        "Alp::ProcessRollback rollback to timestamp smaller than GVT! Agent {0}, timestamp {1}, current GVT {2}",
        agent_id, rollback_to_timestamp, this->GetGvt());
    exit(1);
  }
  spdlog::warn("LOGRB {} {} {}", agent_id, rollback_to_timestamp, GetAlpManagedAgentLvt(agent_id));
  // check if need to rollback alp managed lvt history
  if (rollback_to_timestamp < GetAlpManagedAgentLvt(agent_id)) {
    this->SetAlpManagedAgentLvt(agent_id, rollback_to_timestamp);
  }
  this->SetAgentLocalLvtAfterRollback(agent_id, rollback_to_timestamp);

  spdlog::debug("Agent {0} rollback to timestamp {1}", agent_id, rollback_to_timestamp);

  /*
   We first need to remove all those events with time stamp greater than
   that of the roll-back which have been generated and not yet sent.  i.e.,
   remove all messages in the sendQ which have time greater than roll-back
   time. We do this before generating the anti-messages to prevent them from
   also being deleted from the sendQ.
   */
  fSendMessageQueue->RemoveMessages(pRollbackMessage->GetOriginalAgent(), pRollbackMessage->GetTimestamp());
  //spdlog::warn("All events removed");
  /*
   Next stage of roll-back is to process the sentList, sending out
   anti-messages for all the messages that have been sent out by this
   Alp after the time of the roll-back.
   */
  list<SharedStateMessage *> rollbackMessagesInSendList = RollbackSendList(pRollbackMessage->GetTimestamp(),
                                                                           pRollbackMessage->GetOriginalAgent());
  for (list<SharedStateMessage *>::iterator iter = rollbackMessagesInSendList.begin();
       iter != rollbackMessagesInSendList.end();) {
    if (fGVT > (*iter)->GetTimestamp()) {
      ostringstream out;
      (*iter)->Serialise(out);
      spdlog::error("Alp::ProcessRollback({}): rollback before GVT, time {}, message {}", GetRank(),
                    (*iter)->GetTimestamp(), out.str());

    } else {
      switch ((*iter)->GetType()) {
        case SINGLEREADMESSAGE : {
          SingleReadMessage *singleReadMessage = static_cast<SingleReadMessage *> (*iter);
          SingleReadAntiMessage *antiSingleReadMessage = new SingleReadAntiMessage();
          antiSingleReadMessage->SetOrigin(GetRank());
          antiSingleReadMessage->SetDestination(GetParentClp());
          antiSingleReadMessage->SetTimestamp(singleReadMessage->GetTimestamp());
          // Mattern colour?
          antiSingleReadMessage->SetNumberOfHops(0);
          antiSingleReadMessage->SetRollbackTag(pRollbackMessage->GetRollbackTag());
          antiSingleReadMessage->SetOriginalAgent(singleReadMessage->GetOriginalAgent());
          antiSingleReadMessage->SetSsvId(singleReadMessage->GetSsvId());
          antiSingleReadMessage->SendToLp(this);
        }
          break;
        case WRITEMESSAGE : {
          WriteMessage *writeMessage = static_cast<WriteMessage *> (*iter);
          WriteAntiMessage *antiWriteMessage = new WriteAntiMessage();
          antiWriteMessage->SetOrigin(GetRank());
          antiWriteMessage->SetDestination(GetParentClp());
          antiWriteMessage->SetTimestamp(writeMessage->GetTimestamp());
          // Mattern colour?
          antiWriteMessage->SetNumberOfHops(0);
          antiWriteMessage->SetRollbackTag(pRollbackMessage->GetRollbackTag());
          antiWriteMessage->SetOriginalAgent(writeMessage->GetOriginalAgent());
          antiWriteMessage->SetSsvId(writeMessage->GetSsvId());
          antiWriteMessage->SendToLp(this);
        }
          break;
        case RANGEQUERYMESSAGE : {
          RangeQueryMessage *rangeQueryMessage = static_cast<RangeQueryMessage *> (*iter);
          RangeQueryAntiMessage *antiRangeQueryMessage = new RangeQueryAntiMessage();
          antiRangeQueryMessage->SetOrigin(GetRank());
          antiRangeQueryMessage->SetDestination(GetParentClp());
          antiRangeQueryMessage->SetTimestamp(rangeQueryMessage->GetTimestamp());
          // Mattern colour?
          antiRangeQueryMessage->SetNumberOfHops(0);
          antiRangeQueryMessage->SetRollbackTag(pRollbackMessage->GetRollbackTag());
          antiRangeQueryMessage->SetOriginalAgent(rangeQueryMessage->GetOriginalAgent());
          antiRangeQueryMessage->SetRange(rangeQueryMessage->GetRange());
          antiRangeQueryMessage->SetIdentifier(rangeQueryMessage->GetIdentifier());
          antiRangeQueryMessage->SendToLp(this);
        }
          break;
        default :
          ostringstream out;
          (*iter)->Serialise(out);
          spdlog::warn("Alp::ProcessRollback({}): Unsuitable message found to create antimessage from: {}", GetRank(),
                       out.str());
          break;
      }
    }
    //spdlog::warn("Anti message sent");
    // I don't need to free the memory for value for WriteMessages as that was freed by SendThread when the message
    // was send originally. It was never stored in the send-list anyway.
    delete *iter;
    rollbackMessagesInSendList.erase(iter++);
  }


  SetCancelFlag(agent_id, false);
  agent_response_map_[agent_id] = nullptr;
  agent_response_message_id_map_[agent_id] = 0;
  // restart agent
  spdlog::debug("Agent restarting");
  agent->Restart();
  spdlog::debug("Agent restarted");
  return true;
}

int Alp::GetParentClp() const {
  return fParentClp;
}

unsigned long Alp::GetLvt() const {
  unsigned long minimum_agent_lvt = ULONG_MAX;
  for (auto iter:agent_lvt_map_) {
    unsigned long agentLvt = iter.second;
    if (agentLvt < minimum_agent_lvt) {
      minimum_agent_lvt = agentLvt;
      spdlog::debug("Alp::GetLvt() new minimum: {}, {}, rank {}", iter.first, agentLvt, this->GetRank());
    }
  }
  assert(minimum_agent_lvt != ULONG_MAX);
  spdlog::debug("Alp::GetLvt() LVT {}, rank {}", minimum_agent_lvt, this->GetRank());

  return minimum_agent_lvt;
}

void Alp::SetGvt(unsigned long pGvt) {
  spdlog::warn("LOGGVT {},{}", this->GetRank(), pGvt);
  fGVT = pGvt;
  ClearSendList(pGvt);
  ClearRollbackTagList(pGvt);
  CleanUpAgentLvtHistory(pGvt);
}

unsigned long Alp::GetNewMessageId() const {
  return message_id_handler_->GetNextID();
}

bool Alp::HasAgent(unsigned long agent_id) {
  return managed_agents_.find(agent_id) != managed_agents_.end();
}

unsigned long Alp::GetAlpManagedAgentLvt(unsigned long agent_id) const {
  auto result = agent_lvt_map_.find(agent_id);
  if (result != agent_lvt_map_.end()) {
    return result->second;
  }
  spdlog::critical("Agent LVT not in map! id {0}", agent_id);
  exit(1);
}

bool Alp::SetAlpManagedAgentLvt(unsigned long agent_id, unsigned long newLvt) {
  auto lvt_it = agent_lvt_map_.find(agent_id);
  if (lvt_it == agent_lvt_map_.end()) {
    spdlog::critical("Alp::SetAlpManagedAgentLvt({}, {}) agent not found!", agent_id, newLvt);
    exit(1);
  }

  lvt_it->second = newLvt;
  return true;

}

bool Alp::UpdateAgentLvtToAlp(unsigned long agentId, unsigned long newLvt) {
  unsigned long oldLvt = this->GetAlpManagedAgentLvt(agentId);
  if (oldLvt < newLvt) {
    this->SetAlpManagedAgentLvt(agentId, newLvt);
    this->RecordAgentLvtHistory(agentId, newLvt);
    return true;
  }
  spdlog::error("Alp::UpdateAgentLvtToAlp({}, {}): new {} <= old {}", agentId, newLvt, newLvt, oldLvt);

  return false;
}

bool Alp::RecordAgentLvtHistory(unsigned long agent_id, unsigned long timestamp) {
  auto lvt_history_it = agent_lvt_history_map_.find(agent_id);
  if (timestamp > lvt_history_it->second.back()) {
    lvt_history_it->second.push_back(timestamp);
    return true;
  }
  spdlog::error("Alp::RecordAgentLvtHistory({}, {}): not inserted, new {} <= lvt history largest {}", agent_id,
                timestamp, timestamp,
                lvt_history_it->second.back());

  return false;
}

void Alp::Initialise() {
  spdlog::debug("ALP initialized, {0}", this->GetRank());
  // Nothing yet
}

void Alp::Finalise() {
  spdlog::info("Entering ALP finalise");

  for (auto a:managed_agents_) {
    a.second->Join(); // wait for all agents to exit
  }
  SendEndMessage();


  fMPIInterface->StopSimulation();
  fMPIInterface->Join();
}

void Alp::StartAllAgents() {
  spdlog::debug("total {0} agents to start", managed_agents_.size());
  for (auto i:managed_agents_) {
    i.second->Start();

  }
}

bool Alp::TerminationCondition() const {

  return Lp::TerminationCondition();
}

void Alp::SendEndMessage() {
  EndMessage *endMessage = new EndMessage();
  endMessage->SetOrigin(GetRank());
  endMessage->SetDestination(GetParentClp());
  endMessage->SetSenderAlp(GetRank());
  endMessage->SendToLp(this);
}

void Alp::SetCancelFlag(unsigned long agent_id, bool flag) {
  agent_cancel_flag_map_[agent_id] = flag;
}

bool Alp::GetCancelFlag(unsigned long agent_id) {
  return agent_cancel_flag_map_[agent_id];
}

unsigned long Alp::RollbackAgentLvtHistory(unsigned long agentId, unsigned long timestamp) {
  /**
   * Given rollback message timestamp, erase agent lvt history which is larger than this timestamp
   * Returns the timestamp agent should rollback to**/

  auto &agent_lvt_history_list = agent_lvt_history_map_[agentId];
  spdlog::debug("Alp::RollbackAgentLvtHistory({}, {}), initial size: {}", agentId, timestamp,
                agent_lvt_history_list.size());
  for (auto i:agent_lvt_history_list) {
    spdlog::debug("Agent {}, have history ts {}", agentId, i);
  }
  // get first LVT <= rollback, delete all histories afterward ()
  agent_lvt_history_list.erase(
      std::upper_bound(agent_lvt_history_list.begin(), agent_lvt_history_list.end(), timestamp),
      agent_lvt_history_list.end());
  spdlog::debug("Alp::RollbackAgentLvtHistory: Agent {}, rollback to {}, size after rb: {}", agentId,
                agent_lvt_history_list.back(), agent_lvt_history_list.size());

  return agent_lvt_history_list.back();
}

Agent *Alp::GetAgent(unsigned long agentId) {
  if (managed_agents_.find(agentId) == managed_agents_.end()) {
    spdlog::critical("Agent {} not found in ALP {}!", agentId, this->GetRank());
    exit(1);
  }
  return managed_agents_[agentId];
}

bool Alp::SetAgentLocalLvtAfterRollback(unsigned long agent_id, unsigned long newLvt) {
  this->GetAgent(agent_id)->SetLVT(newLvt);
  return true;
}

bool Alp::CleanUpAgentLvtHistory(unsigned long timestamp) {
  for (auto &i:agent_lvt_history_map_) {
    auto &agent_lvt_history_list = i.second;
    agent_lvt_history_list.erase(
        agent_lvt_history_list.begin(),
        std::lower_bound(agent_lvt_history_list.begin(), agent_lvt_history_list.end(), timestamp)
    );
  }
  return true;
}

map<unsigned long, list<unsigned long>> Alp::GetAgentTimeHistoryMap() const {
  return this->agent_lvt_history_map_;
}


