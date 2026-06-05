#include "quadedge.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/matrix.hpp>
#include <utility>
#include <algorithm>

Edge* Edge::sym() {
    return (localIndex < 2) ? (this + 2) : (this - 2); 
}

Edge* Edge::rotCCW() {
    return (localIndex < 3) ? (this + 1) : (this - 3);
}

Edge* Edge::rotCW() {
    return (localIndex > 0) ? (this - 1) : (this + 3);
}

Edge* Edge::Onext() {
    return next;
}

Edge* Edge::Oprev() {
    return rotCCW()->Onext()->rotCCW();
}

Edge* Edge::Dnext() {
    return sym()->Onext()->sym();
}

Edge* Edge::Dprev() {
    return rotCW()->Onext()->rotCW();
}

Edge* Edge::Lnext() {
    return rotCW()->Onext()->rotCCW();
}

Edge* Edge::Lprev() {
    return Onext()->sym();
}

Edge* Edge::Rnext() {
    return rotCCW()->Onext()->rotCW();
}

Edge* Edge::Rprev() {
    return sym()->Onext();
}

void Edge::setOnext(Edge *newOnext) {
    this->next = newOnext;
}

Edge* QuadEdge::makeEdge() {
    edgeRecords.push_back(new EdgeRecord());
    return edgeRecords.back()->edges;
}

void QuadEdge::splice(Edge *a, Edge *b) {
	Edge* alpha = a->Onext()->rotCCW();
	Edge* beta = b->Onext()->rotCCW();

	Edge* t1 = b->Onext();
	Edge* t2 = a->Onext();
	Edge* t3 = beta->Onext();
	Edge* t4 = alpha->Onext();

	a->setOnext(t1);
	b->setOnext(t2);
	alpha->setOnext(t3);
	beta->setOnext(t4);
}

void Edge::setOrig(int orig) {
    origin = orig;
}
void Edge::setDest(int dest) {
    sym()->origin = dest;
}

int Edge::orig() {
    return origin;
}
int Edge::dest() {
    return sym()->orig();
}
int Edge::index() {
    return localIndex;
}

static auto compareXY(const std::vector<Node> &nodes) {
    return [&nodes] (const unsigned int &a, const unsigned int &b) {
        glm::dvec2 aval = nodes.at(a).position;
        glm::dvec2 bval = nodes.at(b).position;

        if(aval.x < bval.x) {
            return true;
        } else if(aval.x > bval.x) {
            return false;
        } else {
            return aval.y < bval.y;
        }
    };
}

QuadEdge::QuadEdge(const std::shared_ptr<std::vector<Node>> nodes) : nodes(nodes) {
}

void QuadEdge::triangulate() {
    std::vector<unsigned int> ind;
    ind.reserve(nodes->size());

    for(int i = 0; i < nodes->size(); i++) {
        ind.push_back(i);
    }

    std::sort(ind.begin(), ind.end(), compareXY(*nodes));
    this->delaunay(ind.data(), ind.size());
}

Edge* QuadEdge::connect(Edge *a, Edge *b) {
    Edge *e = makeEdge();

//  printf("connected %d and %d\n", a->dest(), b->orig());
    e->setOrig(a->dest());
    e->setDest(b->orig());

    splice(e, a->Lnext());
    splice(e->sym(), b);
    return e;
}

void QuadEdge::deleteEdge(Edge *e) {
    splice(e, e->Oprev());
    splice(e->sym(), e->sym()->Oprev());

    e->setOrig(-1);
    e->setDest(-1);

    EdgeRecord* raw = (EdgeRecord*)(e - (e->index()));
    auto pos = std::find(edgeRecords.begin(), edgeRecords.end(), raw);

    if(pos != edgeRecords.end()) {
        edgeRecords.erase( pos);
    }
}

void QuadEdge::swap(Edge *e) {
    Edge *a = e->Oprev();
    Edge *b = e->sym()->Oprev();

    splice(e, a);
    splice(e->sym(), b);
    splice(e, a->Lnext());
    splice(e->sym(), b->Lnext());

    e->setOrig(a->dest());
    e->setDest(b->dest());
}

long double triangleDet(glm::dvec2 a, glm::dvec2 b, glm::dvec2 c) {
    const long double a_[] = {a.x, a.y};
    const long double b_[] = {b.x, b.y};
    const long double c_[] = {c.x, c.y};

    return ((a_[0] - b_[0])*(a_[1] - c_[1]) - (a_[0] - c_[0])*(a_[1] - b_[1]));
}

bool CCW(glm::dvec2 a, glm::dvec2 b, glm::dvec2 c) {
    return triangleDet(a, b, c) > glm::epsilon<double>();
}

bool inCircle(glm::dvec2 a, glm::dvec2 b, glm::dvec2 c, glm::dvec2 t) {
    a -= t;
    b -= t;
    c -= t;

    const double a_sq = a.x*a.x + a.y*a.y;
    const double b_sq = b.x*b.x + b.y*b.y;
    const double c_sq = c.x*c.x + c.y*c.y;

    const long double det0 = a_sq * ((long double)b.x*c.y - (long double)b.y*c.x);
    const long double det1 = b_sq * ((long double)a.x*c.y - (long double)a.y*c.x);
    const long double det2 = c_sq * ((long double)a.x*b.y - (long double)a.y*b.x);

    return (det0 - det1 + det2) > glm::epsilon<double>();
}

bool QuadEdge::rightOf(Edge *e, glm::dvec2 p) {
    return CCW(p, destOf(e).position, origOf(e).position);
}

bool QuadEdge::leftOf(Edge *e, glm::dvec2 p) {
    return CCW(p, origOf(e).position, destOf(e).position);
}

bool QuadEdge::isValid(Edge *e, Edge *basel) {
    return rightOf(basel, destOf(e).position);
}

std::pair<Edge*, Edge*> QuadEdge::makeLine(unsigned int aInd, unsigned int bInd) {
    Edge *e_ab = makeEdge();

    e_ab->setOrig(aInd);
    e_ab->setDest(bInd);

    return std::make_pair(e_ab, e_ab->sym());
}

std::pair<Edge*, Edge*> QuadEdge::makeTriangle(unsigned int aInd, unsigned int bInd, unsigned int cInd) {
    Edge *e_ab = makeEdge();
    Edge *e_bc = makeEdge();

    e_ab->setOrig(aInd);
    e_ab->setDest(bInd);
    e_bc->setOrig(bInd);
    e_bc->setDest(cInd);

    splice(e_ab->sym(), e_bc);

    Edge *e_ac;

    if(CCW(nodes->at(aInd).position, nodes->at(bInd).position, nodes->at(cInd).position)) {

        e_ac = connect(e_bc, e_ab);
        return std::make_pair(e_ab, e_bc->sym());

    } else if(CCW(nodes->at(aInd).position, nodes->at(cInd).position, nodes->at(bInd).position)) {

        e_ac = connect(e_bc, e_ab);
        return std::make_pair(e_ac->sym(), e_ac);

    } else {
        return std::make_pair(e_ab, e_bc->sym());
    }
}

std::pair<Edge*, Edge*> QuadEdge::delaunay(unsigned int *nodes, int size) {
    auto &nodeInds = nodes;

    if(size == 2) {
        return makeLine(nodeInds[0], nodeInds[1]);
    } else if(size == 3) {
        return makeTriangle(nodeInds[0], nodeInds[1], nodeInds[2]);
    }

    auto ldo_ldi = delaunay(nodes, size / 2);
    auto ldi = ldo_ldi.second;
    auto ldo = ldo_ldi.first;

    auto rdo_rdi = delaunay(nodes + (size / 2), size - (size / 2));
    auto rdi = rdo_rdi.first;
    auto rdo = rdo_rdi.second;

    while(true) {
        if(leftOf(ldi, origOf(rdi).position)) {
            ldi = ldi->Lnext();
        } else if(rightOf(rdi, origOf(ldi).position)) {
            rdi = rdi->Rprev();
        } else {
            break;
        }
    }

    Edge *basel = connect(rdi->sym(), ldi);
    if(ldi->orig() == ldo->orig()) {
        ldo = basel->sym();
    }
    if(rdi->orig() == rdo->orig()) {
        rdo = basel;
    }

    Edge *lcand;
    Edge *rcand;
    while(true) {
        lcand = basel->sym()->Onext();
        if(isValid(lcand, basel)) {
            while(inCircle(destOf(basel).position, origOf(basel).position, destOf(lcand).position, destOf(lcand->Onext()).position)) {
                Edge *tmp = lcand->Onext();
                deleteEdge(lcand);
                lcand = tmp;
            }
        }

        rcand = basel->Oprev();
        if(isValid(rcand, basel)) {
            while(inCircle(destOf(basel).position, origOf(basel).position, destOf(rcand).position, destOf(rcand->Oprev()).position)) {
                Edge *tmp = rcand->Oprev();
                deleteEdge(rcand);
                rcand = tmp;
            }
        }

        bool rcandValid = isValid(rcand, basel);
        bool lcandValid = isValid(lcand, basel);

        if(!rcandValid && !lcandValid) {
            break;

        } else if(!lcandValid || (rcandValid &&
                    inCircle(destOf(lcand).position, origOf(lcand).position, origOf(rcand).position, destOf(rcand).position))) {
            basel = connect(rcand, basel->sym());
        } else {
            basel = connect(basel->sym(), lcand->sym());
        }
    }
    return std::make_pair(ldo, rdo);
}

const Node& QuadEdge::destOf(Edge *e) {
    return nodes->at(e->dest());
}

const Node& QuadEdge::origOf(Edge *e) {
    return nodes->at(e->orig());
}
