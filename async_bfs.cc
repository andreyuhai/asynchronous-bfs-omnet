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

using namespace omnetpp;

class Node : public cSimpleModule {

    std::list<int> children;
    std::list<int> other;
    int my_layer = INT_MAX;
    int parent = -1;

    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void finish() override;
    /*
     * Creates a layer message with the parameters layer and s.
     * If s is skipped, then the default simTime is used.
     */
    layerMessage* createLayerMessage(int layer, simtime_t s = simTime());
    ackMessage* createAckMessage();
    rejectMessage* createRejectMessage();
    void handleLayerMessage(layerMessage *lMsg);
    void handleAckMessage(ackMessage *aMsg);
    void handleRejectMessage(rejectMessage *rMsg);
    void printParentNode();
    void printChildrenNodes();
    void printOtherNodes();
    void disconnectDuplicateConnections();
};

Define_Module(Node);

void Node::handleMessage(cMessage *msg) {
    if(msg->isSelfMessage()) { // If the message is a self message
        bubble("Initiating...");
        layerMessage *lMsg = createLayerMessage(my_layer + 1);
        for(int i = 0; i < gateCount() / 2; i++) {
            if(gate("port$o", i)->isConnected())
                sendDelayed(lMsg->dup(), intuniform(1, 1000), "port$o", i);
        }
        delete lMsg;
    } else {
        switch(msg->getKind()) {
        case 0: { // If the message is a cMessage
        } break;
        case 1: { // If the message is a layerMessage
            layerMessage *lMsg = check_and_cast<layerMessage *>(msg);
            handleLayerMessage(lMsg);
        } break;
        case 2: { // If the message is ackMessage
            ackMessage *aMsg = check_and_cast<ackMessage *>(msg);
            handleAckMessage(aMsg);
        } break;
        case 3: { // If the message is rejectMessage
            rejectMessage *rMsg = check_and_cast<rejectMessage *>(msg);
            handleRejectMessage(rMsg);
        } break;
        }
    }

}

void Node::initialize() {
    disconnectDuplicateConnections();
    // Root sending a self-message to itself to initiate the process
    if(getIndex() == 0) {
        my_layer = 0;
        cMessage *msg = new cMessage("What?");
        scheduleAt(10.0, msg);
    }
}

void Node::finish() {
    EV << "============" << getFullName() << "============" << std::endl;
    printParentNode();
    printChildrenNodes();
    printOtherNodes();
}

layerMessage* Node::createLayerMessage(int layer, simtime_t s) {
    layerMessage *lMessage = new layerMessage;
    lMessage->setLayer(layer);
    lMessage->setTimeFrame(s);
    lMessage->setKind(1);
    return lMessage;
}

ackMessage* Node::createAckMessage() {
    ackMessage *ackMsg = new ackMessage;
    ackMsg->setKind(2);
    return ackMsg;
}

rejectMessage* Node::createRejectMessage() {
    rejectMessage *rMessage = new rejectMessage;
    rMessage->setKind(3);
    return rMessage;
}

/*
 * Compares a layer with a given layer
 */
bool compareLayers(int l1, int l2) {
    return l1 < l2;
}

void Node::handleLayerMessage(layerMessage *lMsg) {
    if(compareLayers(lMsg->getLayer(), my_layer)) {
        my_layer = lMsg->getLayer(); // Change layer size
        parent = lMsg->getArrivalGate()->getIndex();
        char deneme[50];
        sprintf(deneme,"Parent node set to: %s", gate("port$o", parent)->getPathEndGate()->getOwnerModule()->getFullName());
        bubble(deneme);
        ackMessage *aMsg = createAckMessage();
        sendDelayed(aMsg, intuniform(1, 1000), "port$o", lMsg->getArrivalGate()->getIndex());
        for(int i = 0; i < gateCount() / 2; i++) {
            if(i == parent || !gate("port$o", i)->isConnected())
                continue;
            else
                sendDelayed(createLayerMessage(my_layer + 1), intuniform(1, 1000), "port$o", i);
        }
    } else {
        rejectMessage *rMsg = createRejectMessage();
        send(rMsg, "port$o", lMsg->getArrivalGate()->getIndex());
    }
    delete lMsg;
}

void Node::handleAckMessage(ackMessage *aMsg) {
    int index = aMsg->getArrivalGate()->getIndex();
    std::list<int>::iterator it = std::find(other.begin(), other.end(), index);
    if(it != other.end()) {
        EV << "Deleting " << gate("port$o", index)->getPathEndGate()->getOwnerModule()->getFullName() << "from other for " << getFullName() << std::endl;
        other.erase(it);
    }
    children.push_back(index);
    delete aMsg;
}

void Node::handleRejectMessage(rejectMessage *rMsg) {
    int index = rMsg->getArrivalGate()->getIndex();
    if(std::find(other.begin(), other.end(), index) == other.end())
        other.push_back(index);
    delete rMsg;
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

/*
 * Disconnects all the duplicate gate connections of a given node with another node.
 * The function keeps a vector of all the connected node indexes.
 * Then disconnects all the gates that the node already has a gate for.
 * This function ensures that we will not have multiple connections from the same node to a particular node.
 */
void Node::disconnectDuplicateConnections() {
    std::list<int> already_connected;
    for(int i = 0; i < gateCount() / 2; i++) {
        int connectedGateIndex = gate("port$o", i)->getPathEndGate()->getOwnerModule()->getIndex();
        if(std::find(already_connected.begin(), already_connected.end(), connectedGateIndex) == already_connected.end()) {
            already_connected.push_back(connectedGateIndex);
        } else {
            gate("port$i", i)->disconnect();
            gate("port$o", i)->disconnect();
        }
    }
}
