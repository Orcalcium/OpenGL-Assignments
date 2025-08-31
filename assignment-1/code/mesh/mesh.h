class Mesh {
    public:
        virtual float* getMesh() = 0;
        virtual int getVertCount() = 0;
    private:
        float* vertices;
};

class Cube : public Mesh {
    public:
        float* getMesh() override {
            return cube;
        }
        int getVertCount() override{
            return sizeof(cube)/sizeof(float)/5*6;
        }
    private:
        static float cube[180];
};

class Crosshair : public Mesh {
    public:
        float* getMesh() override {
            return crosshair;
        }
        int getVertCount() override{
            return sizeof(crosshair) / sizeof(float);
        }
    private:
        static float crosshair[12];
};