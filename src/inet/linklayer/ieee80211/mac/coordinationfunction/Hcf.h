//
// Copyright (C) 2016 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __INET_HCF_H
#define __INET_HCF_H

#include "inet/linklayer/ieee80211/mac/blockack/OriginatorBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/blockack/RecipientBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/blockack/RecipientBlockAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/channelaccess/Edca.h"
#include "inet/linklayer/ieee80211/mac/channelaccess/Hcca.h"
#include "inet/linklayer/ieee80211/mac/contract/ICoordinationFunction.h"
#include "inet/linklayer/ieee80211/mac/contract/ITx.h"
#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceContext.h"
#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceHandler.h"
#include "inet/linklayer/ieee80211/mac/lifetime/EdcaTransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/AckHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorBlockAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorQoSMacDataService.h"
#include "inet/linklayer/ieee80211/mac/originator/RecoveryProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/RtsProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/TxopProcedure.h"
#include "inet/linklayer/ieee80211/mac/queue/InProgressFrames.h"
#include "inet/linklayer/ieee80211/mac/recipient/CtsProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientQoSMacDataService.h"

namespace inet {
namespace ieee80211 {

class Ieee80211Mac;

/**
 * Implements IEEE 802.11 Hybrid Coordination Function.
 */
class INET_API Hcf : public ICoordinationFunction, public IFrameSequenceHandler::ICallback, public IContentionBasedChannelAccess::ICallback, public IContentionFreeChannelAccess::ICallback, public ITx::ICallback, public IRecoveryProcedure::ICwCalculator, public cSimpleModule
{
    protected:
        Ieee80211Mac *mac = nullptr;

        // Transmission and Reception
        IRx *rx = nullptr;
        ITx *tx = nullptr;

        IRateSelection *rateSelection = nullptr;

        // Channel Access Methods
        Edca *edca = nullptr;
        int numEdcafs = -1;
        Hcca *hcca = nullptr;

        // MAC Data Service
        OriginatorQoSMacDataService *originatorDataService = nullptr;
        RecipientQoSMacDataService *recipientDataService = nullptr;

        // MAC Procedures
        RecipientAckProcedure *recipientAckProcedure = nullptr;
        OriginatorAckProcedure *originatorAckProcedure = nullptr;
        RtsProcedure *rtsProcedure = nullptr;
        CtsProcedure *ctsProcedure = nullptr;
        OriginatorBlockAckProcedure *originatorBlockAckProcedure = nullptr;
        RecipientBlockAckProcedure *recipientBlockAckProcedure = nullptr;
        EdcaTransmitLifetimeHandler *lifetimeHandler = nullptr;
        std::vector<RecoveryProcedure *> edcaRecoveryProcedures;
        RecoveryProcedure *hccaRecoveryProcedure = nullptr;

        // Block Ack Agreement Handlers
        OriginatorBlockAckAgreementHandler *originatorBlockAckAgreementHandler = nullptr;
        RecipientBlockAckAgreementHandler *recipientBlockAckAgreementHandler = nullptr;

        // Ack handler
        std::vector<AckHandler *> edcaAckHandlers;
        AckHandler *hccaAckHandler = nullptr;

        // Tx Opportunity
        std::vector<TxopProcedure*> edcaTxops;
        TxopProcedure *hccaTxop = nullptr;

        // Queues
        std::vector<PendingQueue *> edcaPendingQueues;
        PendingQueue *hccaPendingQueue = nullptr;
        std::vector<InProgressFrames *> edcaInProgressFrames;
        InProgressFrames *hccaInProgressFrame = nullptr;

        // Frame sequence handler
        IFrameSequenceHandler *frameSequenceHandler = nullptr;

        const IIeee80211Mode *referenceMode = nullptr;

        simtime_t sifs = -1;

    protected:
        virtual ~Hcf();
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;

        void startFrameSequence(AccessCategory ac);
        void handleInternalCollision(std::vector<Edcaf*> internallyCollidedEdcafs);

        void sendUp(const std::vector<Ieee80211Frame*>& completeFrames);
        FrameSequenceContext* buildContext(AccessCategory ac);
        virtual bool hasFrameToTransmit();

        virtual void recipientProcessReceivedFrame(Ieee80211Frame *frame);
        virtual void recipientProcessReceivedControlFrame(Ieee80211Frame *frame);
        virtual void recipientProcessReceivedManagementFrame(Ieee80211ManagementFrame *frame);
        virtual void originatorProcessTransmittedManagementFrame(Ieee80211ManagementFrame *mgmtFrame, AccessCategory ac);
        virtual void originatorProcessTransmittedControlFrame(Ieee80211Frame *controlFrame, AccessCategory ac);
        virtual void originatorProcessTransmittedDataFrame(Ieee80211DataFrame *dataFrame, AccessCategory ac);
        virtual void originatorProcessReceivedManagementFrame(Ieee80211ManagementFrame *frame, Ieee80211Frame *lastTransmittedFrame, AccessCategory ac);
        virtual void originatorProcessReceivedControlFrame(Ieee80211Frame *frame, Ieee80211Frame *lastTransmittedFrame, AccessCategory ac);
        virtual void originatorProcessReceivedDataFrame(Ieee80211DataFrame* frame, Ieee80211Frame* lastTransmittedFrame, AccessCategory ac);

    protected:
        // IFrameSequenceHandler::ICallback
        virtual void originatorProcessRtsProtectionFailed(Ieee80211DataOrMgmtFrame *protectedFrame) override;
        virtual void originatorProcessTransmittedFrame(Ieee80211Frame* transmittedFrame) override;
        virtual void originatorProcessReceivedFrame(Ieee80211Frame *frame, Ieee80211Frame *lastTransmittedFrame) override;
        virtual void originatorProcessFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame) override;
        virtual void frameSequenceFinished() override;
        virtual void transmitFrame(Ieee80211Frame *frame, simtime_t ifs) override;
        virtual bool isReceptionInProgress() override;

        // IRecoveryProcedure::ICallback
        virtual int computeCwMin(IRecoveryProcedure *rp);
        virtual int computeCwMax(IRecoveryProcedure *rp);

        // IContentionBasedChannelAccess::ICallback
        virtual void channelGranted(IContentionBasedChannelAccess *channelAccess) override;
        virtual int getCw(IContentionBasedChannelAccess *channelAccess) override;

        // IContentionFreeChannelAccess::ICallback
        virtual void channelGranted(IContentionFreeChannelAccess *channelAccess) override;

        // ITx::ICallback
        virtual void transmissionComplete() override;

    public:
        // ICoordinationFunction
        virtual void processUpperFrame(Ieee80211DataOrMgmtFrame *frame) override;
        virtual void processLowerFrame(Ieee80211Frame *frame) override;

};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_HCF_H
