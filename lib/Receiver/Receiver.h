class Receiver {
    private:
        int addr;
        int readStatus(int, bool);
        int wait(int, bool);
    public:
        Receiver(int addr, int resetpin);
        int sendPatch();
        void loadFlash();
        void boot();
        bool getBootStatus();
        void getPartInfo();
};