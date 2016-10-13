#include <string>
#include <vector>

typedef std::vector<std::string> StringVector;

namespace inet {
using namespace std;

struct HeartBeatRecord {
public:
    int devId = -1; //The id of the device
    string ipAddress = "";
    string macAddress = "";
    StringVector reachableSSIDs;
    int ttl = -1;
};
}
