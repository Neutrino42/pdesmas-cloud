cmake_minimum_required(VERSION 3.10)
project(PDES-MAS)
find_package(MPI REQUIRED)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER mpicxx)
set(PDESMAS_CXX_FLAGS "-DRANGE_QUERIES -DSSV_LOCALISATION -fPIC -Wall -Wextra -pthread -lpthread -Wl,--no-as-needed,-rpath='${ORIGIN}'")

link_libraries(m stdc++ pthread)

set(CMAKE_CXX_FLAGS_DEBUG "${PDESMAS_CXX_FLAGS} -O0 -ggdb -DPDESMAS_DEBUG")

set(CMAKE_CXX_FLAGS_RELEASE "${PDESMAS_CXX_FLAGS} -O3")

include_directories(${JNI_INCLUDE_DIRS})

include_directories(include)
include_directories(include/interface)
include_directories(include/lp)
include_directories(include/messages)
include_directories(include/messages/content)
include_directories(include/parse)
include_directories(include/rangequery)
include_directories(include/routing)
include_directories(include/state)
include_directories(include/types)
include_directories(include/util)
include_directories(include/interface/agent_thread)
add_library(PDESMAS SHARED
        src/interface/HasIDLVTMap.cpp
        src/interface/HasResponseMessage.cpp
        src/interface/HasRollbackTagList.cpp
        src/interface/HasSendList.cpp
        src/interface/Agent.cpp
        src/lp/Alp.cpp
        src/interface/IdentifierHandler.cpp
        src/lp/AccessCostCalculator.cpp
        src/lp/Clp.cpp
        src/lp/GvtCalculator.cpp
        src/lp/Lp.cpp
        src/lp/LpId.cpp
        src/lp/MessageQueue.cpp
        src/lp/MpiInterface.cpp
        src/lp/ReceiveThread.cpp
        src/lp/RollbackList.cpp
        src/lp/RollbackTag.cpp
        src/lp/RQPortScanStatus.cpp
        src/lp/SendThread.cpp
        src/messages/content/HasDestination.cpp
        src/messages/content/HasGVT.cpp
        src/messages/content/HasIdentifier.cpp
        src/messages/content/HasMatternColour.cpp
        src/messages/content/HasMatternCut.cpp
        src/messages/content/HasMessageCount.cpp
        src/messages/content/HasMessageMinimumTime.cpp
        src/messages/content/HasNumberOfHops.cpp
        src/messages/content/HasNumberOfTraverseHops.cpp
        src/messages/content/HasOrigin.cpp
        src/messages/content/HasOriginalAgent.cpp
        src/messages/content/HasRange.cpp
        src/messages/content/HasRedMessageTime.cpp
        src/messages/content/HasRollbackTag.cpp
        src/messages/content/HasSenderAlp.cpp
        src/messages/content/HasSSVID.cpp
        src/messages/content/HasSSVIDValueMap.cpp
        src/messages/content/HasStateVariableMap.cpp
        src/messages/content/HasTimestamp.cpp
        src/messages/content/HasValue.cpp
        src/messages/content/HasWriteStatus.cpp
        src/messages/AbstractMessage.cpp
        src/messages/AntiMessage.cpp
        src/messages/ControlMessage.cpp
        src/messages/EndMessage.cpp
        src/messages/GvtControlMessage.cpp
        src/messages/GvtMessage.cpp
        src/messages/GvtRequestMessage.cpp
        src/messages/GvtValueMessage.cpp
        src/messages/LoadBalancingMessage.cpp
        src/messages/RangeQueryAntiMessage.cpp
        src/messages/RangeQueryMessage.cpp
        src/messages/RangeUpdateMessage.cpp
        src/messages/ResponseMessage.cpp
        src/messages/RollbackMessage.cpp
        src/messages/SharedStateMessage.cpp
        src/messages/SimulationMessage.cpp
        src/messages/SingleReadAntiMessage.cpp
        src/messages/SingleReadMessage.cpp
        src/messages/SingleReadResponseMessage.cpp
        src/messages/StateMigrationMessage.cpp
        src/messages/WriteAntiMessage.cpp
        src/messages/WriteMessage.cpp
        src/messages/WriteResponseMessage.cpp
        src/parse/Initialisor.cpp
        src/rangequery/RangePeriod.cpp
        src/rangequery/RangePeriodListEntry.cpp
        src/rangequery/RangeRoutingTable.cpp
        src/rangequery/RangeTracker.cpp
        src/rangequery/RangeUpdates.cpp
        src/routing/ForwardingTable.cpp
        src/routing/Router.cpp
        src/routing/RouteTable.cpp
        src/routing/RoutingInfo.cpp
        src/state/AbstractValue.cpp
        src/state/ObjectMgr.cpp
        src/state/SharedState.cpp
        src/state/SsvId.cpp
        src/state/StateVariable.cpp
        src/state/WritePeriod.cpp
        src/types/Point.cpp
        src/types/Range.cpp
        src/types/Types.cpp
        src/util/FilePrint.cpp
        src/util/Log.cpp
        src/util/Mutex.cpp
        src/util/Thread.cpp
        src/interface/Simulation.cpp
        src/state/PrivateVariableStorage.cpp)

add_executable(runtest src/testing/TestAgent.cpp src/testing/run.cpp)
add_executable(tileworld src/testing/Tileworld/TileWorldAgent.cpp src/testing/Tileworld/run.cpp)
add_executable(naivemb src/testing/NaiveMailbox/NaiveAgent.cpp src/testing/NaiveMailbox/run.cpp)

target_link_libraries(runtest PDESMAS)
target_link_libraries(tileworld PDESMAS)
target_link_libraries(naivemb PDESMAS)
