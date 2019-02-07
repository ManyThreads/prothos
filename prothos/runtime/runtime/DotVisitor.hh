#pragma once

#include <sstream>
#include <fstream>
#include "Visitor.hh"

class DotVisitor: public Visitor {
public:
    std::string to_string() {
        std::stringstream ss;
        ss << "digraph D {\n";
        ss << "\n";
        ss << "\trankdir=LR;\n";

        for (auto const& x : visitedNodes) {
            ss << "\t" << x.second->getKey() << "[shape = " << typeToShape(x.second->getNodeType()) <<"]\n";
        }
        ss << "\n";
        for (auto const& x : visitedNodes) {
            for (Visitable * v: x.second->getSuccessors()) {
                // retrieve description for each successor
                std::map<Visitable*, NodeDescription *>::iterator it = visitedNodes.find(v);
                NodeDescription * nd = it->second;
                ss << "\t" << x.second->getKey() << " -> " << nd->getKey() << ";\n";
            }
        }
        ss << "}";
        return ss.str();
    }

    void to_file() {
        std::ofstream myfile;
        myfile.open ("flowgraph.dot");
        myfile << to_string();
        myfile.close();
    }

    /**
     * convert nodetype to shapestring
     **/
    static std::string typeToShape(NodeType type) {
        switch(type) {
            case ENotImplemented: return "star"; break;
            case EContinueNode: return "box"; break;
            case EFunctionNode: return "ellipse"; break;
            case ESplitNode: return "trapezium"; break;
            case EJoinNode: return "invtrapezium"; break;
            case ESourceNode: return "doublecircle"; break;
            case EConditionalNode: return "Mcircle"; break;
            default: return "circle";
        }
    }
};

