/*
 Atmel_sam_common/SamSPI.h - Library for the Motate system
 http://github.com/synthetos/motate/

 Copyright (c) 2013 - 2018  Robert Giseburt

 This file is part of the Motate Library.

 This file ("the software") is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2 as published by the
 Free Software Foundation. You should have received a copy of the GNU General Public
 License, version 2 along with the software. If not, see <http://www.gnu.org/licenses/>.

 As a special exception, you may use this file as part of a software library without
 restriction. Specifically, if other files instantiate templates or use macros or
 inline functions from this file, or you compile this file and link it with  other
 files to produce an executable, this file does not by itself cause the resulting
 executable to be covered by the GNU General Public License. This exception does not
 however invalidate any other reasons why the executable file might be covered by the
 GNU General Public License.

 THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef SAMSPI_H_ONCE
#define SAMSPI_H_ONCE

#include "sam.h"

#include "MotatePins.h"
#include "SamCommon.h"
#include "SamDMA.h"
#include <type_traits>

namespace Motate {
    namespace SPI_internal {
        template<class T, const intptr_t val>
        struct deregister {
            static constexpr T const value() { return reinterpret_cast<T>(val); }
            constexpr deregister() {};
            constexpr T const operator->() const { return reinterpret_cast<T>(val); }
            constexpr operator T const() const { return reinterpret_cast<T>(val); }
        };

        #if defined(SPI)
            #define CAN_SPI_PDC_DMA 1

            // This is for the Sam4e
            Spi * const SPI0_DONT_CONFLICT = static_cast<Spi *>(SPI);
            #undef SPI
            Spi * const SPI0_Peripheral = SPI0_DONT_CONFLICT;
            constexpr uint16_t const ID_SPI0_DONT_CONFLICT = ID_SPI;

            #undef ID_SPI
            constexpr uint16_t const ID_SPI0 = ID_SPI0_DONT_CONFLICT;
            constexpr IRQn_Type SPI0_IRQn = SPI_IRQn;

            #define HAS_SPI0
            // This define is because, on this processor, they chose to call it "SPI_Handler" instead
            // and saves an ifdef and duplicated code in the SamSPI.cpp
            #define SPI0_Handler SPI_Handler
        #elif defined(SPI0)
            // This is for the Sam3x and SamS70
            static const intptr_t SPI0_DONT_CONFLICT {(intptr_t)SPI0};
            #undef SPI0
            constexpr deregister<Spi *, SPI0_DONT_CONFLICT> SPI0_Peripheral;

            constexpr uint16_t const ID_SPI0_DONT_CONFLICT = ID_SPI0;
            #undef ID_SPI0
            constexpr uint16_t const ID_SPI0 = ID_SPI0_DONT_CONFLICT;

            #define HAS_SPI0
        #endif

        #if defined(SPI1)
            // This is for the Sam3x and SamS70
            static const intptr_t SPI1_DONT_CONFLICT {(intptr_t)SPI1};
            #undef SPI1
            constexpr deregister<Spi *, SPI1_DONT_CONFLICT> SPI1_Peripheral;

            constexpr uint16_t const ID_SPI1_DONT_CONFLICT = ID_SPI1;
            #undef ID_SPI1
            constexpr uint16_t const ID_SPI1 = ID_SPI1_DONT_CONFLICT;

            #define HAS_SPI1
        #endif

        template<int8_t spiPeripheralNumber>
        struct SPIInfo {}; // SPIInfo <generic>

        template<>
        struct SPIInfo<0>
        {
            static constexpr bool exists = true;
            // static constexpr Spi * const spi_fn() { return SPI0_DONT_CONFLICT.value(); }
            static constexpr auto spi = SPI0_Peripheral;
            static constexpr uint32_t peripheralId = ID_SPI0;
            static constexpr IRQn_Type spiIRQ = SPI0_IRQn;
        }; // SPIInfo <0>

    #if defined(HAS_SPI1)
        template<>
        struct SPIInfo<1>
        {
            static constexpr bool exists = true;
            // static constexpr Spi * const spi_fn() { return SPI1_DONT_CONFLICT.value(); }
            static constexpr auto const spi = SPI1_Peripheral;
            static constexpr uint32_t peripheralId = ID_SPI1;
            static constexpr IRQn_Type spiIRQ = SPI1_IRQn;
        }; // SPIInfo <1>
    #endif

    }; // Motate::SPI_internal

    // We're deducing if there's a SPI0 and it has a PDC
    // Notice that this relies on defines set up in SamCommon.h
    #if defined(HAS_SPI0)

    #if defined(CAN_SPI_PDC_DMA)

    #else
    // if !defined(CAN_SPI_PDC_DMA)
    #pragma mark DMA_XDMAC SPI implementation

    template<uint8_t spiPeripheralNumber>
    struct DMA_XDMAC_hardware<Spi*, spiPeripheralNumber> : Motate::SPI_internal::SPIInfo<spiPeripheralNumber>
    {
        using SPI_internal::SPIInfo<spiPeripheralNumber>::spi;
        auto peripheralId() const { return SPI_internal::SPIInfo<spiPeripheralNumber>::peripheralId; }
        typedef char* buffer_t;
    };

    template<uint8_t spiPeripheralNumber>
    struct DMA_XDMAC_TX_hardware<Spi*, spiPeripheralNumber> : virtual DMA_XDMAC_hardware<Spi*, spiPeripheralNumber>, virtual DMA_XDMAC_common {
        using DMA_XDMAC_hardware<Spi*, spiPeripheralNumber>::spi;
        using DMA_XDMAC_hardware<Spi*, spiPeripheralNumber>::peripheralId;

        static constexpr uint8_t const xdmaTxPeripheralId()
        {
            switch (spiPeripheralNumber) {
                case (0): return 1;
                case (1): return 3;
            };
            return 0;
        };
        static constexpr uint8_t const xdmaTxChannelNumber()
        {
            switch (spiPeripheralNumber) {
                case (0): return  18;
                case (1): return  20;
            };
            return 0;
        };
        static constexpr XdmacChid * const xdmaTxChannel()
        {
            return xdma()->XDMAC_CHID + xdmaTxChannelNumber();
        };
        static constexpr volatile void * const xdmaPeripheralTxAddress()
        {
            return &spi->SPI_TDR;
        };
    };

    template<uint8_t spiPeripheralNumber>
    struct DMA_XDMAC_RX_hardware<Spi*, spiPeripheralNumber> : virtual DMA_XDMAC_hardware<Spi*, spiPeripheralNumber>, virtual DMA_XDMAC_common {
        using DMA_XDMAC_hardware<Spi*, spiPeripheralNumber>::spi;
        using DMA_XDMAC_hardware<Spi*, spiPeripheralNumber>::peripheralId;

        static constexpr uint8_t const xdmaRxPeripheralId()
        {
            switch (spiPeripheralNumber) {
                case (0): return 2;
                case (1): return 4;
            };
            return 0;
        };
        static constexpr uint8_t const xdmaRxChannelNumber()
        {
            switch (spiPeripheralNumber) {
                case (0): return 19;
                case (1): return 21;
            };
            return 0;
        };
        static constexpr XdmacChid * const xdmaRxChannel()
        {
            return xdma()->XDMAC_CHID + xdmaRxChannelNumber();
        };
        static constexpr volatile void * const xdmaPeripheralRxAddress()
        {
            return &spi->SPI_RDR;
        };
    };

    // Construct a DMA specialization that uses the PDC
    template<uint8_t periph_num>
    struct DMA<Spi*, periph_num> : DMA_XDMAC_RX<Spi*, periph_num>, DMA_XDMAC_TX<Spi*, periph_num> {
        // nothing to do here, except for a constxpr constructor
        constexpr DMA(const std::function<void(uint16_t)> &handler) : DMA_XDMAC_RX<Spi*, periph_num>{handler}, DMA_XDMAC_TX<Spi*, periph_num>{handler} {};

        using DMA_XDMAC_common::xdma;
        using DMA_XDMAC_RX<Spi*, periph_num>::xdmaRxChannelNumber;
        using DMA_XDMAC_TX<Spi*, periph_num>::xdmaTxChannelNumber;

        void setInterrupts(const uint16_t interrupts) const
        {
            DMA_XDMAC_common::setInterrupts(interrupts);

            if (interrupts != Interrupt::Off) {
                if (interrupts & Interrupt::OnTxTransferDone) {
                    DMA_XDMAC_TX<Spi*, periph_num>::startTxDoneInterrupts();
                } else {
                    DMA_XDMAC_TX<Spi*, periph_num>::stopTxDoneInterrupts();
                }

                if (interrupts & Interrupt::OnRxTransferDone) {
                    DMA_XDMAC_RX<Spi*, periph_num>::startRxDoneInterrupts();
                } else {
                    DMA_XDMAC_RX<Spi*, periph_num>::stopRxDoneInterrupts();
                }
            } else {
                DMA_XDMAC_TX<Spi*, periph_num>::stopTxDoneInterrupts();
                DMA_XDMAC_RX<Spi*, periph_num>::stopRxDoneInterrupts();

            }
        };

        void reset() const {
            DMA_XDMAC_TX<Spi*, periph_num>::resetTX();
            DMA_XDMAC_RX<Spi*, periph_num>::resetRX();
        };

        void enable() const {
            xdma()->XDMAC_GE = (XDMAC_GIE_IE0 << xdmaRxChannelNumber()) | (XDMAC_GIE_IE0 << xdmaTxChannelNumber());
        }

        void disable() const
        {
            xdma()->XDMAC_GD = (XDMAC_GID_ID0 << xdmaRxChannelNumber()) | (XDMAC_GID_ID0 << xdmaRxChannelNumber());
        };
    };
    #endif // defined(CAN_SPI_PDC_DMA)
    #endif // SPI + XDMAC

    #undef HAS_SPI1
    #undef HAS_SPI0


    template<int8_t spiPeripheralNumber>
    struct _SPIHardware : Motate::SPI_internal::SPIInfo<spiPeripheralNumber>
    {
        static_assert(Motate::SPI_internal::SPIInfo<spiPeripheralNumber>::exists,
                "Only _SPIHardware<0> or _SPIHardware<1> is valid on this processor.");

        using Motate::SPI_internal::SPIInfo<spiPeripheralNumber>::spi;
        using Motate::SPI_internal::SPIInfo<spiPeripheralNumber>::peripheralId;
        using Motate::SPI_internal::SPIInfo<spiPeripheralNumber>::spiIRQ;
        const uint8_t spiPeripheralNum = spiPeripheralNumber;

        typedef _SPIHardware<spiPeripheralNumber> this_type_t;

        std::function<void(uint16_t)> _spiInterruptHandler;

#ifndef CAN_SPI_PDC_DMA
        DMA<Spi *, spiPeripheralNumber> dma_ {_spiInterruptHandler};
        constexpr const DMA<Spi *, spiPeripheralNumber> *dma() { return &dma_; };
#endif

        static std::function<void()> _spiInterruptHandlerJumper;
        _SPIHardware() {
            SamCommon::enablePeripheralClock(peripheralId);

            // Softare reset of SPI module
            spi->SPI_CR = SPI_CR_SWRST;
            disable();
        };

        void init() {
//            static bool inited = false;
//            if (inited)
//                return;
//            inited = true;

            // Set last transfer
            spi->SPI_CR = SPI_CR_LASTXFER;

            // Set Mode Register to Master mode
            spi->SPI_MR |= SPI_MR_MSTR;

            // Mode Fault Detection Disabled
            spi->SPI_MR |= SPI_MR_MODFDIS;

            // Ensure Fixed Peripheral Select
            spi->SPI_MR &= (~SPI_MR_PS);

            // Disable all interrupts
            spi->SPI_IDR = 0x7FF;

            // setup interrupt handlers BEFORE setting up DMA, in case it causes an interrupt
            _spiInterruptHandlerJumper = [&]() {
                if (_spiInterruptHandler) {
                    _spiInterruptHandler(getInterruptCause());
                } else {
#if IN_DEBUGGER == 1
                    __asm__("BKPT");
#endif
                }
            };

#ifdef CAN_SPI_PDC_DMA
            // Reset the PDC
            spi->SPI_PTCR = SPI_PTCR_RXTDIS | SPI_PTCR_TXTDIS;
            spi->SPI_RPR = 0;
            spi->SPI_RCR = 0;
            spi->SPI_TPR = 0;
            spi->SPI_TCR = 0;
            spi->SPI_RNPR = 0;
            spi->SPI_RNCR = 0;
            spi->SPI_TNPR = 0;
            spi->SPI_TNCR = 0;
#else
            dma()->reset();
#endif
        };

        // This is to be called by the device, once it detects a "decoded" CS pin
        void setUsingCSDecoder(bool decoder) {
            if (decoder) {
                spi->SPI_MR |= SPI_MR_PCSDEC;
            } else {
                spi->SPI_MR &= ~SPI_MR_PCSDEC;
            }
        }

        void enable() {
            spi->SPI_CR = SPI_CR_SPIEN ;
        };

        void disable() {
            spi->SPI_CR = SPI_CR_SPIDIS;
        };

        void deassert() {
            disable();
        }

        bool setChannel(const uint8_t channel) {
            // if we are transmitting, we cannot switch
            while (!(spi->SPI_SR & SPI_SR_TXEMPTY)) {
                ;
            }

            spi->SPI_MR = (spi->SPI_MR & ~SPI_MR_PCS_Msk) | SPI_MR_PCS(channel);

            enable();
            return true;
        }

        void setChannelOptions(const uint8_t channel, const uint32_t baud, const uint16_t options, uint32_t min_between_cs_delay_ns, uint32_t cs_to_sck_delay_ns, uint32_t between_word_delay_ns) {
            // We derive the baud from the master clock with a divider.
            // We want the closest match *below* the value asked for. It's safer to bee too slow.
            uint32_t new_otions = 0;
            uint32_t old_options = spi->SPI_CSR[channel];

            uint32_t divider = SamCommon::getPeripheralClockFreq() / baud;
            if (divider > 255) {
                divider = 255;
            } else if (divider < 1) {
                divider = 1;
            }

            uint32_t old_divider = (old_options & SPI_CSR_SCBR_Msk) >> SPI_CSR_SCBR_Pos;
            divider = std::max(old_divider, divider); // this gives the slowest rate of the old or new setting
            new_otions |= SPI_CSR_SCBR(divider);

            // We can't really mix these ... they must already be compatible
            // But if we are using a debugger we can throw a wrench in and catch it
#if IN_DEBUGGER == 1
            uint32_t old_polarity = (old_options & SPI_CSR_CPOL);
            if ((old_divider != 0) && (!!(options & kSPIPolarityReversed) != !!old_polarity)) {
                __asm__("BKPT"); // two SPI devices sharing config have different polarity!
            }
#endif
            if (options & kSPIPolarityReversed) {
                new_otions |= SPI_CSR_CPOL;
            }

            // We can't really mix these ... they must already be compatible
            // But if we are using a debugger we can throw a wrench in and catch it
#if IN_DEBUGGER == 1
            uint32_t old_phase = (old_options & SPI_CSR_NCPHA);
            if ((old_divider != 0) && (!!(options & kSPIClockPhaseReversed) != !!old_phase)) {
                __asm__("BKPT"); // two SPI devices sharing config have different phases!
            }
#endif
            if (options & kSPIClockPhaseReversed) {
                new_otions |= SPI_CSR_NCPHA;
            }

            switch (options & kSPIBitsMask) {
                case kSPI9Bit:
                    new_otions |= SPI_CSR_BITS_9_BIT;
                    break;
                case kSPI10Bit:
                    new_otions |= SPI_CSR_BITS_10_BIT;
                    break;
                case kSPI11Bit:
                    new_otions |= SPI_CSR_BITS_11_BIT;
                    break;
                case kSPI12Bit:
                    new_otions |= SPI_CSR_BITS_12_BIT;
                    break;
                case kSPI13Bit:
                    new_otions |= SPI_CSR_BITS_13_BIT;
                    break;
                case kSPI14Bit:
                    new_otions |= SPI_CSR_BITS_14_BIT;
                    break;
                case kSPI15Bit:
                    new_otions |= SPI_CSR_BITS_15_BIT;
                    break;
                case kSPI16Bit:
                    new_otions |= SPI_CSR_BITS_16_BIT;
                    break;

                case kSPI8Bit:
                default:
                    new_otions |= SPI_CSR_BITS_8_BIT;
                    break;
            }

#if IN_DEBUGGER == 1
            uint32_t old_bits = (old_options & SPI_CSR_BITS_Msk);
            if ((old_divider != 0) && ((options & SPI_CSR_BITS_Msk) != old_bits)) {
                __asm__("BKPT"); // two SPI devices have different bit-lengths!
            }
#endif

            // min_between_cs_delay_ns = DLYBCS
            // cs_to_sck_delay_ns = DLYBS
            // between_word_delay_ns = DLYBCT

            // these are in mupliples of MCLK (master clock, a.k.a. SystemCoreClock)
            // we want to round up, not down, so we divide by 100,000,000 instead of
            // 1,000,000,000, then add 5 and then further divide by 10.

            uint32_t dlybcs = (((min_between_cs_delay_ns*SamCommon::getPeripheralClockFreq())/100000000)+5)/10;

            if (dlybcs > 0xff) {
                // Break into the debugger
#if IN_DEBUGGER == 1
                __asm__("BKPT"); // SPI dlybcs is too high!
#endif
            }

            // Always use the larger delay
            uint32_t old_dlybcs = ((spi->SPI_MR & SPI_MR_DLYBCS_Msk) >> SPI_MR_DLYBCS_Pos);
            if ( old_dlybcs < dlybcs ) {
                spi->SPI_MR = (spi->SPI_MR & ~SPI_MR_DLYBCS_Msk) | SPI_MR_DLYBCS(dlybcs);
            }


            uint32_t dlybs = (((cs_to_sck_delay_ns*SamCommon::getPeripheralClockFreq())/100000000)+5)/10;
            if (dlybs > 0xff) {
                // Break into the debugger
#if IN_DEBUGGER == 1
                __asm__("BKPT"); // SPI dlybs is too high!
#endif
            }
            uint32_t old_dlybs = (old_options & SPI_CSR_SCBR_Msk) >> SPI_CSR_SCBR_Pos;
            new_otions |= SPI_CSR_DLYBS(std::max(dlybs, old_dlybs));

            uint32_t dlybct = (((between_word_delay_ns*SamCommon::getPeripheralClockFreq())/100000000)+5)/10;
            if (dlybct > 0xff) {
                // Break into the debugger
#if IN_DEBUGGER == 1
                __asm__("BKPT"); // SPI dlybct is too high!
#endif
            }
            uint32_t old_dlybct = (old_options & SPI_CSR_DLYBCT_Msk) >> SPI_CSR_DLYBCT_Pos;
            new_otions |= SPI_CSR_DLYBCT(std::max(dlybct, old_dlybct));

            // We'll drive CS low after we're done, so we always want this:
            new_otions |= SPI_CSR_CSAAT;

            spi->SPI_CSR[channel] = new_otions;
        };

        /* TEMPORARILY REMOVING DIRECT READ/WRITE/TRANSFER. Can be brought back later */
#if 0
        static int16_t read(const bool lastXfer = false, uint8_t toSendAsNoop = 0) {
            if (!(spi->SPI_SR & SPI_SR_RDRF)) {
                if (spi->SPI_SR & SPI_SR_TXEMPTY) {
                    spi->SPI_TDR = toSendAsNoop;
                    if (lastXfer) {
                        spi->SPI_CR = SPI_CR_LASTXFER;
                    }
                }
                return -1;
            }

            return spi->SPI_RDR;
        }

        static int16_t write(uint8_t value, const bool lastXfer = false) {
            int16_t throw_away;
            // Let's see what the optimizer does with this...
            return write(value, throw_away, lastXfer);
        };

        static int16_t write(uint8_t value, int16_t &readValue, const bool lastXfer = false) {
            while (!(spi->SPI_SR & SPI_SR_TDRE)) {;}

            if (spi->SPI_SR & SPI_SR_RDRF) {
                readValue = spi->SPI_RDR;
            } else {
                readValue = -1;
            }

            if (spi->SPI_SR & SPI_SR_TDRE) {

                spi->SPI_TDR = value;

                if (lastXfer) {
                    spi->SPI_CR = SPI_CR_LASTXFER;
                }

                return 1;
            }

            return -1;
        };

        int16_t transmit(uint8_t channel, const uint16_t data, const bool lastXfer = false) {
            uint32_t data_with_flags = data;

            if (lastXfer)
                data_with_flags |= SPI_TDR_LASTXFER;

            // NOTE: Assumes we DON'T have an external decoder/multiplexer!
            data_with_flags |= SPI_TDR_PCS(~(1<< channel));

            while (!(spi->SPI_SR & SPI_SR_TDRE))
                ;
            spi->SPI_TDR = data_with_flags;

            while (!(spi->SPI_SR & SPI_SR_RDRF))
                ;

            uint16_t outdata = spi->SPI_RDR;
            return outdata;
        };
#endif // temporarily removed read/write/transfer


        void setInterruptHandler(std::function<void(uint16_t)> &&handler) {
            _spiInterruptHandler = std::move(handler);
        }

        uint16_t getInterruptCause() {
            uint16_t status = SPIInterrupt::Unknown;

            // Notes from experience:
            // This processor will sometimes allow one of these bits to be set,
            // even when there is no interrupt requested, and the setup conditions
            // don't appear to be done.
            // The simple but unfortunate fix is to verify that the Interrupt Mask
            // calls for that interrupt before considering it as a possible interrupt
            // source. This should be a best practice anyway, really. -Giseburt

            auto SPI_SR_hold = spi->SPI_SR;
            auto SPI_IMR_hold = spi->SPI_IMR;

            if ((SPI_IMR_hold & SPI_IMR_TDRE) && (SPI_SR_hold & SPI_SR_TDRE))
            {
                status |= SPIInterrupt::OnTxReady;
            }
            if ((SPI_IMR_hold & SPI_IMR_RDRF) && (SPI_SR_hold & SPI_SR_RDRF))
            {
                status |= SPIInterrupt::OnRxReady;
            }
#ifdef CAN_SPI_PDC_DMA
            if ((SPI_IMR_hold & SPI_IMR_TXBUFE) && (SPI_SR_hold & SPI_SR_TXBUFE))
            {
                status |= SPIInterrupt::OnTxTransferDone;
            }
            if ((SPI_IMR_hold & SPI_IMR_RXBUFF) && (SPI_SR_hold & SPI_SR_RXBUFF))
            {
                status |= SPIInterrupt::OnRxTransferDone;
            }
#else
            if (dma()->inTxBufferEmptyInterrupt())
            {
                status |= SPIInterrupt::OnTxTransferDone;
            }
            if (dma()->inRxBufferFullInterrupt())
            {
                status |= SPIInterrupt::OnRxTransferDone;
            }
#endif
            return status;
        }

        void setInterrupts(const uint16_t interrupts) {
            if (interrupts != SPIInterrupt::Off) {

                if (interrupts & SPIInterrupt::OnTxReady) {
                    spi->SPI_IER = SPI_IER_TDRE;
                } else {
                    spi->SPI_IDR = SPI_IDR_TDRE;
                }
                if (interrupts & SPIInterrupt::OnRxReady) {
                    spi->SPI_IER = SPI_IER_RDRF;
                } else {
                    spi->SPI_IDR = SPI_IDR_RDRF;
                }

#ifdef CAN_SPI_PDC_DMA
                if (interrupts & SPIInterrupt::OnTxTransferDone) {
                    spi->SPI_IER = SPI_IER_TXBUFE;
                } else {
                    spi->SPI_IDR = SPI_IDR_TXBUFE;
                }
                if (interrupts & SPIInterrupt::OnRxTransferDone) {
                    spi->SPI_IER = SPI_IER_RXBUFF;
                } else {
                    spi->SPI_IDR = SPI_IDR_RXBUFF;
                }
#else
                if (interrupts & SPIInterrupt::OnRxTransferDone) {
                    dma()->startRxDoneInterrupts();
                } else {
                    dma()->stopRxDoneInterrupts();
                }
                if (interrupts & SPIInterrupt::OnTxTransferDone) {
                    dma()->startTxDoneInterrupts();
                } else {
                    dma()->stopTxDoneInterrupts();
                }
#endif

                /* Set interrupt priority */
                if (interrupts & SPIInterrupt::PriorityHighest) {
                    NVIC_SetPriority(spiIRQ, 0);
                }
                else if (interrupts & SPIInterrupt::PriorityHigh) {
                    NVIC_SetPriority(spiIRQ, 1);
                }
                else if (interrupts & SPIInterrupt::PriorityMedium) {
                    NVIC_SetPriority(spiIRQ, 2);
                }
                else if (interrupts & SPIInterrupt::PriorityLow) {
                    NVIC_SetPriority(spiIRQ, 3);
                }
                else if (interrupts & SPIInterrupt::PriorityLowest) {
                    NVIC_SetPriority(spiIRQ, 4);
                }

                NVIC_EnableIRQ(spiIRQ);
            } else {

                NVIC_DisableIRQ(spiIRQ);
            }
        };

#ifdef CAN_SPI_PDC_DMA
        void _enableOnTXTransferDoneInterrupt() {
            spi->SPI_IER = SPI_IER_TXBUFE;
        };

        void _disableOnTXTransferDoneInterrupt() {
            spi->SPI_IDR = SPI_IDR_TXBUFE;
        };

        void _enableOnRXTransferDoneInterrupt() {
            spi->SPI_IER = SPI_IER_RXBUFF;
        };

        void _disableOnRXTransferDoneInterrupt() {
            spi->SPI_IDR = SPI_IDR_RXBUFF;
        };

        uint8_t getMessageSlotsAvailable() {
            uint8_t count = 0;
            if (spi->SPI_RCR > 0) { count++; }
            if (spi->SPI_NRCR > 0) { count++; }
            return count;
        };

        // start transfer of message
        bool startTransfer(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t size) {
            // We need to clear the read buffer or it won't clock anything...
            if (spi->SPI_SR & SPI_SR_RDRF) {
                /*uint16_t dont_care =*/ spi->SPI_RDR;
            }

            if ((spi->SPI_RCR == 0) && (spi->SPI_TCR == 0)) {
                spi->SPI_PTCR = SPI_PTCR_RXTDIS | SPI_PTCR_TXTDIS;

                uint32_t PTCR_prep = 0;

                // setup immediate PDC transfer
                if (rx_buffer != nullptr) {
                    spi->SPI_RPR = (uint32_t)rx_buffer;
                    spi->SPI_RCR = size;
                    _enableOnRXTransferDoneInterrupt();
                    PTCR_prep = SPI_PTCR_RXTEN;
                } else {
                    spi->SPI_RPR = 0;
                    spi->SPI_RCR = 0;
//                    _toss_next_read = true;
                }
                if (tx_buffer != nullptr) {
                    spi->SPI_TPR = (uint32_t)tx_buffer;
                    spi->SPI_TCR = size;
//                    _enableOnTXTransferDoneInterrupt();
                    PTCR_prep |= SPI_PTCR_TXTEN;
                } else {
                    spi->SPI_TPR = 0;
                    spi->SPI_TCR = 0;
                }

                // enable both transfers - we use zero size to diable, but this
                // allows the next transfer to be of a different direction set
                spi->SPI_PTCR = PTCR_prep;
                return true;
            }
//            else if ((spi->SPI_RNCR == 0) && (spi->SPI_TNCR == 0)) {
//                // setup next PDC transfer
//                if (rx_buffer != nullptr) {
//                    spi->SPI_RNPR = (uint32_t)rx_buffer;
//                    spi->SPI_RNCR = size;
//                } else {
//                    spi->SPI_RNPR = 0;
//                    spi->SPI_RNCR = 0;
//                }
//                if (tx_buffer != nullptr) {
//                    spi->SPI_TNPR = (uint32_t)tx_buffer;
//                    spi->SPI_TNCR = size;
//                } else {
//                    spi->SPI_TNPR = 0;
//                    spi->SPI_TNCR = 0;
//                    // HMM, how to handle _toss_next_read here?
//                }
//
//                // current transfer should already be enabled...
//                return true;
//            }

            // We didn't set anything up...
            return false;
        }
#else
        void _enableOnTXTransferDoneInterrupt() {
            dma()->startTxDoneInterrupts();
        };

        void _disableOnTXTransferDoneInterrupt() {
            dma()->stopTxDoneInterrupts();
        };

        void _enableOnRXTransferDoneInterrupt() {
            dma()->startRxDoneInterrupts();
        };

        void _disableOnRXTransferDoneInterrupt() {
            dma()->stopRxDoneInterrupts();
        };

        uint8_t getMessageSlotsAvailable() {
            uint8_t count = 0;
            if (dma()->doneWriting() && dma()->doneReading()) { count++; }
//            if (dma()->doneWritingNext()) { count++; }
            return count;
        };

        bool doneWriting() {
            return dma()->doneWriting();
        };
        bool doneReading() {
            return dma()->doneReading();
        };

        // start transfer of message
        bool startTransfer(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t size) {
            bool is_setup = false;
            dma()->setInterrupts(Interrupt::Off);
            if (rx_buffer != nullptr) {
                const bool handle_interrupts = false;
                const bool include_next = false;
                is_setup = dma()->startRXTransfer(rx_buffer, size, handle_interrupts, include_next);
                if (!is_setup) { return false; } // fail early
            }
            if (tx_buffer != nullptr) {
                const bool handle_interrupts = false;
                const bool include_next = false;
                is_setup = dma()->startTXTransfer(tx_buffer, size, handle_interrupts, include_next);
            }
            if (is_setup) {
                enable();
                //dma()->enable();
                dma()->setInterrupts(Interrupt::OnTxTransferDone | Interrupt::OnRxTransferDone);
            }
            return is_setup;
        }
#endif // CAN_SPI_PDC_DMA

        // abort transfer of message
        // TODO

        // get transfer status
        // TODO
    };

#if 0
    pin_number _default_MISOPinNumber = ReversePinLookup<'A', 25>::number;
    pin_number _default_MOSIPinNumber = ReversePinLookup<'A', 26>::number;
    pin_number _default_SCKPinNumber = ReversePinLookup<'A', 27>::number;

    template<int8_t spiCSPinNumber, int8_t spiMISOPinNumber=_default_MISOPinNumber, int8_t spiMOSIPinNumber=_default_MOSIPinNumber, int8_t spiSCKSPinNumber=_default_SCKPinNumber>
    struct SPI {
        typedef SPIChipSelectPin<spiCSPinNumber> csPinType;
        csPinType csPin;

        SPIMISOPin<spiMISOPinNumber> misoPin;
        SPIMOSIPin<spiMOSIPinNumber> mosiPin;
        SPISCKPin<spiSCKSPinNumber> sckPin;

        static _SPIHardware< misoPin::spiNum > hardware;
        static const uint8_t spiPeripheralNum() { return csPinType::moduleId; };
        static const uint8_t spiChannelNumber() { return SPIChipSelectPin<spiCSPinNumber>::csOffset; };

        static Spi * const spi { return hardware.spi; };
        static const uint32_t peripheralId { return hardware.peripheralId; };
        static const IRQn_Type spiIRQ { return hardware.spiIRQ; };



        SPI(const uint32_t baud = 4000000, const uint16_t options = kSPI8Bit | kSPIMode0) {
            hardware.init();
            init(baud, options, /*fromConstructor =*/ true);
        };

        void init(const uint32_t baud, const uint16_t options, const bool fromConstructor=false) {
            setOptions(baud, options, fromConstructor);
        };

//        void setOptions(const uint32_t baud, const uint16_t options, const bool fromConstructor=false) {
//            // We derive the baud from the master clock with a divider.
//            // We want the closest match *below* the value asked for. It's safer to bee too slow.
//
//            uint16_t divider = SystemCoreClock / baud;
//            if (divider > 255)
//                divider = 255;
//            else if (divider < 1)
//                divider = 1;
//
//            spi->SPI_CSR[spiChannelNumber()] = (options & (SPI_CSR_NCPHA | SPI_CSR_CPOL | SPI_CSR_BITS_Msk)) | SPI_CSR_SCBR(divider) | SPI_CSR_DLYBCT(1) | SPI_CSR_CSAAT;
//
//            // Should be a non-op for already-enabled devices.
//            hardware.enable();
//
//        };

        bool setChannel() {
            return hardware.setChannel(spiChannelNumber());
        };

//        uint16_t getOptions() {
//            return spi->SPI_CSR[spiChannelNumber()]/* & (SPI_CSR_NCPHA | SPI_CSR_CPOL | SPI_CSR_BITS_Msk)*/;
//        };

        int16_t readByte(const bool lastXfer = false, uint8_t toSendAsNoop = 0) {
            return hardware.read(lastXfer, toSendAsNoop);
        };

        // WARNING: Currently only reads in bytes. For more-that-byte size data, we'll need another call.
        int16_t read(const uint8_t *buffer, const uint16_t length) {
            if (!setChannel())
                return -1;


            int16_t total_read = 0;
            int16_t to_read = length;
            const uint8_t *read_ptr = buffer;

            bool lastXfer = false;

            // BLOCKING!!
            while (to_read > 0) {

                if (to_read == 1)
                    lastXfer = true;

                int16_t ret = read(lastXfer);

                if (ret >= 0) {
                    *read_ptr++ = ret;
                    total_read++;
                    to_read--;
                }
            };

            return total_read;
        };

        int16_t writeByte(uint16_t data, const bool lastXfer = false) {
            return hardware.write(data, lastXfer);
        };

        int16_t writeByte(uint8_t data, int16_t &readValue, const bool lastXfer = false) {
            return hardware.write(data, lastXfer);
        };

        void flush() {
            hardware.disable();
            hardware.enable();
        };

        // WARNING: Currently only writes in bytes. For more-that-byte size data, we'll need another call.
        int16_t write(const uint8_t *data, const uint16_t length, bool autoFlush = true) {
            if (!setChannel())
                return -1;

            int16_t total_written = 0;
            const uint8_t *out_buffer = data;
            int16_t to_write = length;

            bool lastXfer = false;

            // BLOCKING!!
            do {
                if (autoFlush && to_write == 1)
                    lastXfer = true;

                int16_t ret = write(*out_buffer, lastXfer);

                if (ret > 0) {
                    out_buffer++;
                    total_written++;
                    to_write--;
                }
            } while (to_write);

            // HACK! Autoflush forced...
            if (autoFlush && total_written > 0)
                flush();

            return total_written;
        }
    };

#endif



    template <pin_number csBit0PinNumber, pin_number csBit1PinNumber, pin_number csBit2PinNumber, pin_number csBit3PinNumber>
    struct SPIChipSelectPinMux {
        // These pins may be null, but if they're not, they must be valid CS pins
        static_assert(SPIChipSelectPin<csBit0PinNumber>::is_real || Pin<csBit0PinNumber>::isNull(),
                      "SPIChipSelectPinMux bit 0 pin is not on a real CS pin.");
        static_assert(SPIChipSelectPin<csBit1PinNumber>::is_real || Pin<csBit1PinNumber>::isNull(),
                      "SPIChipSelectPinMux bit 0 pin is not on a real CS pin.");
        static_assert(SPIChipSelectPin<csBit2PinNumber>::is_real || Pin<csBit2PinNumber>::isNull(),
                      "SPIChipSelectPinMux bit 0 pin is not on a real CS pin.");
        static_assert(SPIChipSelectPin<csBit3PinNumber>::is_real || Pin<csBit3PinNumber>::isNull(),
                      "SPIChipSelectPinMux bit 0 pin is not on a real CS pin.");

        struct _dummyCSPin { static constexpr uint8_t csNumber = 0; };

        template<pin_number n>
        using dummyOrCSPin = typename std::conditional<SPIChipSelectPin<n>::is_real, SPIChipSelectPin<n>, _dummyCSPin>::type;

        // create and initialize the CS pins we use
        dummyOrCSPin<csBit0PinNumber> bit0Pin;
        dummyOrCSPin<csBit1PinNumber> bit1Pin;
        dummyOrCSPin<csBit2PinNumber> bit2Pin;
        dummyOrCSPin<csBit3PinNumber> bit3Pin;

        typedef SPIChipSelectPinMux<csBit0PinNumber, csBit1PinNumber, csBit2PinNumber, csBit3PinNumber> type;

        constexpr uint8_t computeCsValue(uint8_t cs) {
            uint8_t csValue = 0;
            if (cs & (1<<0)) { csValue |= (1<<bit0Pin.csNumber); }
            if (cs & (1<<1)) { csValue |= (1<<bit1Pin.csNumber); }
            if (cs & (1<<2)) { csValue |= (1<<bit2Pin.csNumber); }
            if (cs & (1<<3)) { csValue |= (1<<bit3Pin.csNumber); }
            return csValue;
        };

        // Now provide a subobject that offers corect csNumber and csValue for each muxed output
        // We have three names that are confusing:

        // cs = the number we're going to call this one externally, using the order of the bits we provided
        //  IOW, cs of 3  is what you get when bit0Pin and bit1Pin are HIGH

        // csNumber = the internal cs number used by the hardware. All cs where csNumber is the same MUST share settings.
        // csValue  = the internal value provided to the spi hardware (PCS for the Sam chips)
        struct SPIChipSelect {
            const uint8_t csValue;
            const uint8_t csNumber;
            const bool usesDecoder = true;

            SPIChipSelect(SPIChipSelectPinMux * const pm, const uint8_t cs) : csValue{pm->computeCsValue(cs)}, csNumber {(uint8_t)(csValue >> 2)} {
            };

            // delete copy constructor
            SPIChipSelect(const SPIChipSelect &) = delete;

            // build move constructor
            SPIChipSelect(SPIChipSelect && other) : csValue{other.csValue}, csNumber{other.csNumber} {};
        };

        constexpr SPIChipSelect getCS(const uint8_t cs) { return {this, cs}; };
    };


    // SPIGetHardware is just a pass-through for now
    template <pin_number spiMISOPinNumber, pin_number spiMOSIPinNumber, pin_number spiSCKPinNumber>
    using SPIGetHardware = _SPIHardware<SPIMISOPin<spiMISOPinNumber>::spiNum>;
}

#endif /* end of include guard: SAMSPI_H_ONCE */
