#include <patternmodel.h>

int getmodeltype(const std::string filename) {
    unsigned char null;
    unsigned char model_type;
    unsigned char model_version;
    std::ifstream * f = new std::ifstream(filename.c_str());
    f->read( (char*) &null, sizeof(char));        
    f->read( (char*) &model_type, sizeof(char));        
    f->read( (char*) &model_version, sizeof(char));        
    f->close();
    delete f;
    if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNMODEL) ))  {
        return 0;
    } else {
        return model_type;
    }
}

