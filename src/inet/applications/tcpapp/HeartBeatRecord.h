namespace inet {
struct HeartBeatRecord {
public:
    int devId; //The id of the wlan0 for consistency with the service discovery app
    const char *ipAddress;
    const char *macAddress;
    int ttl;
};
}
