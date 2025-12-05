#pragma once

struct IF_ID;
struct ID_EX;

struct HazardResult {
    bool stall = false;
};

class HazardUnit {
public:
    HazardResult detect(const IF_ID& if_id, const ID_EX& id_ex);
};
