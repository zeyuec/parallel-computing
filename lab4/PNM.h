#ifndef PNM_H

#define PNM_H
#include <fstream> 

using namespace std;

class PNM {
    
public:
    
    PNM() {
        
    }

    void init(int w, int h) {
        red_ = new int[w*h];
        green_ = new int[w*h];
        blue_ = new int[w*h];
    }

    bool readFromFile(string fileName) {
        ifstream image(fileName.c_str());
        string line;

        if (!image.is_open()) {
            return false;
        }

        // get mode
        image >> line;
        // getline(image, line);
        if (line != "P3") {
            return false;
        }
        mode_ = 3;

        // get width and height
        image >> width >> height;
        init(width, height);

        // get max
        image >> max_;

        // get RGB matrix
        for (int i=0; i<height; i++) {
            for (int j=0; j<width; j++) {
                image >> red_[i*width+j] >> green_[i*width+j] >> blue_[i*width+j];
            }
        }
        
        image.close();
        return true;
    }

    bool writeToFile(string fileName) {
        ofstream image(fileName.c_str());
        int count = 0;
        if (image.is_open()) {
            image << "P3" << endl;
            image << width << ' ' << height << endl;
            image << 255 << endl;
            for (int i=0; i<height; i++) {
                for (int j=0; j<width; j++) {
                    image << red_[i*width+j] << ' ' << green_[i*width+j] << ' ' << blue_[i*width+j] << ' ';
                    if (count ++ > 6) {
                        image << endl;
                        count = 0;
                    }
                }
            }
            image.close();
        } else {
            return false;
        }
    }

    int& r(unsigned int x, unsigned int y) { 
        return red_[x*width + y]; 
    }
    int& g(unsigned int x, unsigned int y) {
        return green_[x*width + y];
    }
    int& b(unsigned int x, unsigned int y) {
        return blue_[x*width + y];
    }

    PNM& operator=( PNM& other) {
        width = other.width;
        height = other.height;
        for (int i=0; i<height; i++) {
            for (int j=0; j<width; j++) {
                r(i, j) = other.r(i, j);
                g(i, j) = other.g(i, j);
                b(i, j) = other.b(i, j);
            }
        }
        return *this;
    }    
    
    int width, height;
    
private:
    int *red_, *green_, *blue_;
    int mode_, max_;
    
};

#endif
