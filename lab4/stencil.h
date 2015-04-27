#ifndef STENCIL_H

#define STENCIL_H

#include <fstream> 

class Stencil {
public:

    Stencil() {
    }

    void init(int p) {
        data_ = new double[(2*p+1) * (2*p+1)];
    }

    bool readFromFile(string fileName) {
        ifstream image(fileName.c_str());
        if (!image.is_open()) {
            return false;
        }
        
        int width, height;
        image >> width >> height;
        r = (width-1)/2;

        init(r);

        for (int i=0; i<height; i++) {
            for (int j=0; j<width; j++) {
                image >> data_[i*width + j];
            }
        }
        image.close();
        return true;
    }

    double& data(unsigned int x, unsigned int y) {
        return data_[x*(2*r+1) + y];
    }
    
    int r;

private:
    double *data_;
};

#endif
