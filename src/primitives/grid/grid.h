#ifndef GRID_H
#define GRID_H

#include "primitives/primitive.h"
class Grid : public Primitive {
    private:
    void setupVertices();
    public:
    Grid();
    void draw(glm::mat4 vMat, glm::mat4 pMat, glm::vec3) override;
};

#endif