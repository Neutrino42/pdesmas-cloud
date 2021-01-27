//
// Created by pill on 19-6-26.
//

#ifndef PDES_MAS_TILEWORLDAGENT_H
#define PDES_MAS_TILEWORLDAGENT_H

#include <interface/Agent.h>

using namespace pdesmas;
using namespace std;

class TileWorldAgent : public Agent {
public:
  enum Object {
    HOLE, AGENT, TILE, OBSTACLE, NUL
  };
  enum {
    IS_TILE_CARRYING = 10001,
    IS_EN_ROUTE = 10002,
  };

  TileWorldAgent(unsigned long start_time, unsigned long end_time, unsigned long agent_id,
                 unsigned int world_size_w, unsigned int world_size_h, unsigned int sense_range, unsigned int seed);

  void Cycle() override;

  Object GetObjectTypeFromSsvId(SsvId &ssv_id);

private:

  unsigned int kSenseRange;
  unsigned long kLocationSsvId;
  unsigned int world_size_w;
  unsigned int world_size_h;
  unsigned int rand_seed;
  bool tile_carry=false;
  int gvtCount=0;
};


#endif //PDES_MAS_TILEWORLDAGENT_H
