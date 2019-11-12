/*
 * async_bfs.cc
 *
 *  Created on: Nov 6, 2019
 *      Author: burak
 */

#include <omnetpp.h>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <list>
#include "ack_m.h"
#include "layer_m.h"
#include "reject_m.h"

#define DELAYED true            // Choose whether messages should be delivered with a delay or not

using namespace omnetpp;

class Node : public cSimpleModule {
    simtime_t last_creation_time_processed = 0;
    std::list<int> children;    // List to keep our children nodes
    std::list<int> other;       // List to keep our other nodes
    int my_layer = INT_MAX;     // When a node is created its layer is set to INT_MAX
    int parent = -1;            // Parent is set to -1 initially which will let us distinguish which nodes have parents and not.

    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void finish() override;

    layerMessage* createLayerMessage(int layer, simtime_t s = simTime());   // Creates a layerMessage with given parameters
    ackMessage* createAckMessage();                                         // Creates an ackMessage
    rejectMessage* createRejectMessage();                                   // Creates a rejectMessage

    void handleLayerMessage(layerMessage *lMsg);                            // Handles layerMessage(s) received
    void handleAckMessage(ackMessage *aMsg);                                // Handles ackMessage(s) received
    void handleRejectMessage(rejectMessage *rMsg);                          // Handles rejectMessage(s) received

    void printParentNode();                                                 // Prints out the node's parent node
    void printChildrenNodes();                                              // Prints out the node's children nodes
    void printOtherNodes();                                                 // Prints out the node's other nodes
    void printLayer();                                                      // Prints out the node's layer

    bool setParent(layerMessage *lMsg);
    void setParentPathColor();
    void undoParentPathColor();
    void hideOtherNodes();
public:
    int getParent() { return parent; }
};

Define_Module(Node);

void Node::handleMessage(cMessage *msg) {
    if(msg->isSelfMessage()) { // If the message is a self message
        bubble("Initiating...");
        layerMessage *lMsg = createLayerMessage(my_layer + 1);
        for(int i = 0; i < gateCount() / 2; i++) {
            if(DELAYED)
                sendDelayed(lMsg->dup(), intuniform(1, 1000), "port$o", i);
            else
            send(lMsg->dup(), "port$o", i);
        }
        delete lMsg;
    } else {
        switch(msg->getKind()) {
        case 0: {                                                       // If the message is a cMessage
        } break;
        case 1: {                                                       // If the message is a layerMessage
            layerMessage *lMsg = check_and_cast<layerMessage *>(msg);   // Cast the received msg pointer to its appropriate type
            handleLayerMessage(lMsg);                                   // Call appropriate handler function for the message
        } break;
        case 2: {                                                       // If the message is ackMessage
            ackMessage *aMsg = check_and_cast<ackMessage *>(msg);       // Cast the received msg pointer to its appropriate type
            handleAckMessage(aMsg);                                     // Call appropriate handler function for the message
        } break;
        case 3: {                                                       // If the message is rejectMessage
            rejectMessage *rMsg = check_and_cast<rejectMessage *>(msg); // Cast the received msg pointer to its appropriate type
            handleRejectMessage(rMsg);                                  // Call appropriate handler function for the message
        } break;
        }
    }
}

void Node::initialize() {
    // Root scheduling a self-message to initiate the process
    if(getIndex() == 0) {
        my_layer = 0;
        getDisplayString().parse("i=,red");
        cMessage *msg = new cMessage;
        scheduleAt(10.0, msg);
    }
}

/*
 * Creates a layerMessage with the given arguments and returns a pointer to it.
 * simtime_t s parameter is optional, if not given simTime() is used instead.
 */
layerMessage* Node::createLayerMessage(int layer, simtime_t s) {
    layerMessage *lMessage = new layerMessage;
    lMessage->setLayer(layer);
    lMessage->setTimeFrame(s);
    lMessage->setKind(1);
    return lMessage;
}

/*
 * Creates an Ack message setting its `kind` to 2
 * so that we can distinguish the message from cMessage and others.
 * Returns a pointer to created ackMessage.
 */
ackMessage* Node::createAckMessage() {
    ackMessage *ackMsg = new ackMessage;
    ackMsg->setKind(2);
    return ackMsg;
}

/*
 * Creates a Reject message setting its `kind` to 3
 * so that we can distinguis the message from cMessage and others.
 * Returns a pointer to created rejectMessage.
 */
rejectMessage* Node::createRejectMessage() {
    rejectMessage *rMessage = new rejectMessage;
    rMessage->setKind(3);
    return rMessage;
}

/*
 * Compares two layers and returns true if l1 < l2, false otherwise.
 */
bool compareLayers(int l1, int l2) {
    return l1 < l2;
}

/*
 * We handle received message here using other message handler functions declared above.
 */
void Node::handleLayerMessage(layerMessage *lMsg) {

    EV << "HANDLE LAYER MESSAGE IN =========================================" << getFullName() << " FROM " << lMsg->getSenderGate()->getOwnerModule()->getFullName()<< std::endl;
    if(setParent(lMsg)) {     // If setParent return true, which means our layer has been changed

        char bubble_msg[50];
        sprintf(bubble_msg,"Parent node set to: %s", gate("port$o", parent)->getPathEndGate()->getOwnerModule()->getFullName());
        bubble(bubble_msg);
        ackMessage *aMsg = createAckMessage();
        send(aMsg, "port$o", lMsg->getArrivalGate()->getIndex());
        for(int i = 0; i < gateCount() / 2; i++) {
            if(i == parent)
                continue;
            else
                if(DELAYED)
                    sendDelayed(createLayerMessage(my_layer + 1), intuniform(1, 1000), "port$o", i);
                else
                    send(createLayerMessage(my_layer + 1), "port$o", i);
        }
    } else {
        int index = lMsg->getArrivalGate()->getIndex();
        std::list<int>::const_iterator it = std::find(children.begin(), children.end(), index);
        Node *k = check_and_cast<Node*>(gate("port$o", index)->getPathEndGate()->getOwnerModule());
        if( getIndex() != k->gate("port$o", k->getParent())->getPathEndGate()->getOwnerModule()->getIndex()) {
            if(it != children.end()) {
                children.erase(it);
                EV << getFullName() << "'in childrendan " << gate("port$o", index)->getPathEndGate()->getOwnerModule()->getFullName() << " silindi." << std::endl;
            }

            if(std::find(other.begin(), other.end(), index) == other.end()) {
                other.push_back(index);
                EV << getFullName() << "'in othersina " << gate("port$o", index)->getPathEndGate()->getOwnerModule()->getFullName() << " eklendi." << std::endl;
            }

            rejectMessage *rMsg = createRejectMessage();
            send(rMsg, "port$o", lMsg->getArrivalGate()->getIndex());
        }
    }
    delete lMsg;
}

/*
 * Deletes the index of the port that has received the acknowledge message from the `other` list, if it exists in the list.
 * Adds the index of the port that has received the acknowledge message to `children` list, if it doesn't exist in the list already.
 */
void Node::handleAckMessage(ackMessage *aMsg) {
    int index = aMsg->getArrivalGate()->getIndex();
    EV << "HANDLE ACK MESSAGE IN =====================================================" << getFullName() << std::endl;
    std::list<int>::const_iterator it = std::find(other.begin(), other.end(), index);
    if(it != other.end()) {
        other.erase(it);
        EV << getFullName() << "'in othersindan " << gate("port$o", index)->getPathEndGate()->getOwnerModule()->getFullName() << " silindi." << std::endl;;
    } else {
        EV << getFullName() << "'in othersindan " << gate("port$o", index)->getPathEndGate()->getOwnerModule()->getFullName() << " zaten yoktu o yuzden silinmedi." << std::endl;;
    }

    if(std::find(children.begin(), children.end(), index) == children.end()) {
        children.push_back(index);
        EV << getFullName() << "'in childrenina " << gate("port$o", index)->getPathEndGate()->getOwnerModule()->getFullName() << " eklendi." << std::endl;;
    } else {
        EV << getFullName() << "'in childrenininda zaten " << gate("port$o", index)->getPathEndGate()->getOwnerModule()->getFullName() << " vardi eklenmedi." << std::endl;;
    }


    delete aMsg;
}

void Node::handleRejectMessage(rejectMessage *rMsg) {
    int index = rMsg->getArrivalGate()->getIndex();
    /*
     * Depending on the delay times, sometimes when a node receives a reject message from a node.
     * Sender node might already be the parent node of the receiving node.
     * Thus we are checking before adding it to our `other` list.
     */
//    if(index != parent) {
        std::list<int>::const_iterator it;
        if((it = std::find(children.begin(), children.end(), index)) != children.end())
            children.erase(it);

        if(std::find(other.begin(), other.end(), index) == other.end())
            other.push_back(index);
//    }
    delete rMsg;
}



/*
 * Compares the layer value of a layerMessage with the node's actual layer.
 * If the layer value from the message is smaller, then my_layer variable is set to the layer value from the received message.
 * Returns true if layer value has been changed, false otherwise.
 */
bool Node::setParent(layerMessage *lMsg) {
    if(compareLayers(lMsg->getLayer(), my_layer)) {     // If my_layer is greater than the layer in the received message,
        if(parent != -1) {
            undoParentPathColor();
            other.push_back(parent);
        }
        my_layer = lMsg->getLayer();                    // change my_layer to layer in the received message
        parent = lMsg->getArrivalGate()->getIndex();    // Change parent to the index of the port that the message has arrived through.
        EV << getFullName() << "'ina parent olarak " << gate("port$o", parent)->getPathEndGate()->getOwnerModule()->getFullName() << " eklendi" << std::endl;
        std::list<int>::const_iterator it;
        if((it = std::find(other.begin(), other.end(), parent)) != other.end()) {
            other.erase(it);
            EV << getFullName() << "'in othersindan" << gate("port$o", parent)->getPathEndGate()->getOwnerModule()->getFullName() << " silindi" << std::endl;
        }

        setParentPathColor();
        return true;
    }

    return false;
}

/*
 * Changes the color of the path which is connected to parent node back to default.
 * We are using this if our parent node has changed, which means we need to update our path colors as well.
 */
void Node::undoParentPathColor() {
    gate("port$o", parent)->getDisplayString().parse("ls=black,1");
    int index = gate("port$o", parent)->getPathEndGate()->getIndex();
    gate("port$o", parent)->getPathEndGate()->getOwnerModule()->gate("port$o", index)->getDisplayString().parse("ls=black,1");
}

void Node::setParentPathColor() {
    gate("port$o", parent)->getDisplayString().parse("ls=red,3");
    int index = gate("port$o", parent)->getPathEndGate()->getIndex();
    gate("port$o", parent)->getPathEndGate()->getOwnerModule()->gate("port$o", index)->getDisplayString().parse("ls=red,3");
}

void Node::hideOtherNodes() {
    std::list<int>::const_iterator it = other.begin();
    for(; it != other.end(); it++) {
        gate("port$o", *it)->getDisplayString().parse("ls=,0");

    }
}

/*
 * Prints out the parent node.
 */
void Node::printParentNode() {
    if(parent != -1)
        EV << getFullName() << "'s parent node is: " << gate("port$o", parent)->getPathEndGate()->getOwnerModule()->getFullName() << std::endl;
}

/*
 * Prints out children nodes iterating over children vector.
 */
void Node::printChildrenNodes() {
    EV << getFullName();
    if(!children.empty()) {
        EV << "'s children nodes are:" << std::endl;
        for(std::list<int>::const_iterator it = children.begin(); it != children.end(); it++)
            EV << "\t" << gate("port$o", *it)->getPathEndGate()->getOwnerModule()->getFullName() << std::endl;
    } else {
        EV << " does not have any children." << std::endl;
    }
}

/*
 * Prints out other nodes iterating over other vector.
 */
void Node::printOtherNodes() {
    EV << getFullName();
    if(other.begin() != other.end()) {
        EV << "'s other nodes are: " << std::endl;
        for(std::list<int>::const_iterator it = other.begin(); it != other.end(); it++)
            EV << "\t" << gate("port$o", *it)->getPathEndGate()->getOwnerModule()->getFullName() << std::endl;
    } else {
        EV << " does not have any other node." << std::endl;
    }
}

void Node::printLayer() {
    EV << getFullName() << "'s layer is: " << my_layer << std::endl;
}

/*
 * When the simulation ends we print out all the information from nodes.
 * So that we see whose parent is who and etc.
 */
void Node::finish() {
//    hideOtherNodes();
    EV << "============" << getFullName() << "============" << std::endl;
    printParentNode();
    printChildrenNodes();
    printOtherNodes();
    printLayer();
}
