#pragma once
#include "femmesh.h"
#include "quadtree.h"
#include <vector>

struct EdgeRecord;

class Edge {
    private:
        Edge *next;
        int localIndex;
        int origin;

    public:
        void setOrig(int orig);
        void setDest(int dest);

        int orig();
        int dest();
        int index();

        Edge(int index) : localIndex(index), origin(0) {}
        Edge* sym();

        Edge* rotCCW();
        Edge* rotCW(); // Rot^(-1)

        Edge* Onext();
        Edge* Oprev();
        Edge* Dnext();
        Edge* Dprev();

        Edge* Lnext();
        Edge* Lprev();
        Edge* Rnext();
        Edge* Rprev();

        void setOnext(Edge *newOnext);
};

struct EdgeRecord {
    Edge edges[4];

    //set the appropriate indices
    EdgeRecord() : edges{Edge(0), Edge(1), Edge(2), Edge(3)} {
        edges[0].setOnext(&edges[0]);
        edges[1].setOnext(&edges[3]);
        edges[2].setOnext(&edges[2]);
        edges[3].setOnext(&edges[1]);
    }
};

class QuadEdge {
    public:
        QuadEdge(const std::shared_ptr<std::vector<Node>> nodes);

        Edge* makeEdge();
        Edge* makeEdge(int origin);
        void splice(Edge *a, Edge *b);
        Edge* connect(Edge *a, Edge *b);
        void deleteEdge(Edge *e);
        void swap(Edge *e);

        const Node& destOf(Edge *e);
        const Node& origOf(Edge *e);
        
        void triangulate();

        std::pair<Edge*, Edge*> delaunay(unsigned int *nodes, int size);
        const std::shared_ptr<std::vector<Node>> nodes;
        std::vector<EdgeRecord*> edgeRecords;
    private:

        bool rightOf(Edge *e, glm::dvec2 p);
        bool leftOf(Edge *e, glm::dvec2 p);
        bool isValid(Edge *e, Edge *basel);

        std::pair<Edge*, Edge*> makeTriangle(unsigned int aInd, unsigned int bInd, unsigned int cInd);
        std::pair<Edge*, Edge*> makeLine(unsigned int aInd, unsigned int bInd);

};
