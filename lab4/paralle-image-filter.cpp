#include <cstdlib>
#include <iostream>
#include <omp.h>
#include "PNM.h"
#include "stencil.h"
#include "gaussian-blur.h"

using namespace std;

int main(int argc, char **argv) {
    if (argc != 6) {
        cout << "Usage: <input_image> <stecil> <repeat_count> <thread_count> <output_image>" << endl;
        return 1;
    }

    string inputImageName(argv[1]);
    string stencilName(argv[2]);
    int repeat = atoi(argv[3]);
    int threadCount = atoi(argv[4]);
    string outputImageName(argv[5]);

    cout << "Repeat Count = " << repeat << endl;
    cout << "Thread Count = " << threadCount << endl << endl;
    
    // read image
    PNM image;
    if (!image.readFromFile(inputImageName)) {
        cerr << "Load Image Error!" << endl;
        return 1;
    }
    cout << "Image Loaded." << endl;

    // read stencil
    Stencil stencil;
    if (!stencil.readFromFile(stencilName)) {
        cerr << "Load Stencil Error!" << endl;
        return 1;
    }
    cout << "Stencil Loaded." << endl;

    // gaussian blur
    PNM retImage;
    retImage.width = image.width;
    retImage.height = image.height;
    retImage.init(image.width, image.height);
    
    cout << "Bluring..." << endl;
    for (int i=0; i<repeat; i++) {
        cout << "Round " << i+1 << "..." << endl;
        gaussianBlur(image, retImage, threadCount, 0, image.height-1, stencil);
        image = retImage;
    }
    cout << "Done." << endl;
    
    // output
    retImage.writeToFile(outputImageName);
    cout << outputImageName << " Saved" << endl;
    return 0;
}
