#pragma once

#include "rbx/Boost.hpp"

namespace RBX{namespace Graphics{
typedef char dumyy4350583408;

class VisualEngine;
class IndexBuffer;
class VertexLayout;
class Technique;
class ShaderProgram;

struct EmitterShared
{
    VisualEngine*               visualEngine;
    std::shared_ptr< VertexBuffer >  vbuf;
    std::shared_ptr< IndexBuffer >   ibuf;
    std::shared_ptr< VertexLayout >  vlayout;
    shared_ptr<ShaderProgram>   shaders[5];
    G3D::Random                 rnd;
    bool                        colorBGR;

    void*                       vblock;
    void*                       vbptr;
    void*                       vbend;

    void  init( VisualEngine* ve );
    void* lock( int* retIndex, int cnt);
    void  flush();
};


}}
