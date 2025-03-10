//
// Generated file, do not edit! Created by nedtool 5.5 from reject.msg.
//

#ifndef __REJECT_M_H
#define __REJECT_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0505
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



/**
 * Class generated from <tt>reject.msg:1</tt> by nedtool.
 * <pre>
 * message rejectMessage
 * {
 * }
 * </pre>
 */
class rejectMessage : public ::omnetpp::cMessage
{
  protected:

  private:
    void copy(const rejectMessage& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const rejectMessage&);

  public:
    rejectMessage(const char *name=nullptr, short kind=0);
    rejectMessage(const rejectMessage& other);
    virtual ~rejectMessage();
    rejectMessage& operator=(const rejectMessage& other);
    virtual rejectMessage *dup() const override {return new rejectMessage(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const rejectMessage& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, rejectMessage& obj) {obj.parsimUnpack(b);}


#endif // ifndef __REJECT_M_H

