#include "audio_out_mqs.hpp"

#include <Audio.h> //some weird dependency issue prevents me from directly including the file below--not sure?
#include <utility/imxrt_hw.h> //setting audio clock--might drop this code directly into this file

//========================= STATIC VARIABLE INITIALIZATION =========================

DMAChannel Audio_Out_MQS::mqs_dma(false); //don't allocate just yet
DMAMEM __attribute__((aligned(32))) Audio_Out_MQS::Audio_Out_DMA_Mem Audio_Out_MQS::dma_memory;
Context_Callback_Function<void> Audio_Out_MQS::user_cb; //user callback function on DMA half-completion
bool Audio_Out_MQS::dma_mem_write_to_fronthalf = false; //which half of the DMA mem to write to

//=========================== PUBLIC MEMBER FUNCTIONS ======================

void Audio_Out_MQS::init() {

    mqs_dma.begin(true); //from output_mqs.cpp, Allocate the DMA channel first

	//configure clocking to: audio subsystem, I2S3 peripheral, and MQS peripheral
	mqs_configure_clocks();

	//map pin 12 to be an MQS output (MQSL)
	//CORE_PIN10_CONFIG = 2;//B0_00 MQS_RIGHT
	CORE_PIN12_CONFIG = 2;//B0_01 MQS_LEFT

	/*
	 * Special DMA configuration to get the MQS/SAI3 peripheral ticking
	 * 	Doing the DMA configuration via the DMA library
	 * 	Cross checking with the original version of output_mqs.cpp and I think they should be identical
	 */

	//set up the DMA source to be our chunk of memory
	//run from start to finish, head back to the beginning of the buffer after we hit the end of the chunk of memory
	//ensure we're calling the particular overload that transfers four bytes from the source
	mqs_dma.sourceBuffer((const volatile unsigned long*) &dma_memory, sizeof(dma_memory));
	
	//set the destination to be the data register of the I2S3 peripheral
	//don't need to increment any addresses on the destination side, so should be chill to just use this function
	//ensure that we're calling the particular overload that transfers four bytes into the destination
	mqs_dma.destination((volatile unsigned long&) I2S3_TDR0);

	//each transfer has four bytes (i.e. just move one 32-bit word each DMA trigger)
	mqs_dma.transferSize(4);

	//double-buffered DMA, so trigger at half-completion and full completion
	//also attach our static interrupt service routine here
	mqs_dma.interruptAtHalf();
	mqs_dma.interruptAtCompletion();
	mqs_dma.attachInterrupt(Audio_Out_MQS::mqs_isr, App_Constants::MQS_DMA_INT_PRIO);

	//trigger when the SAI3 peripheral wants more data
	mqs_dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI3_TX);
}

void Audio_Out_MQS::start() {
	//start servicing DMA request launched by the I2S3 peripheral
	mqs_dma.enable();

	//and enable the I2S3 transmitter, bit clock, and DMA request generator
	I2S3_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
}

void Audio_Out_MQS::update(std::array<int16_t, App_Constants::PROCESSING_BLOCK_SIZE>& block_in) {
	//just copy over the block into the correct half of the buffer
	//using copy function from standard library to achieve this -- should get efficiently compliled
	//type conversion from int16_t to int32_t should be implicit too and handled as efficiently as possible I think
	if(dma_mem_write_to_fronthalf)
		std::copy(block_in.begin(), block_in.end(), dma_memory.half_buffers.fronthalf.begin());
	else
		std::copy(block_in.begin(), block_in.end(), dma_memory.half_buffers.backhalf.begin());

	//make sure to also flush the cache after writing to any line of memory
	//since DMA can't access the cache, need to flush cache contents out to RAM
	arm_dcache_flush_delete(&dma_memory, sizeof(dma_memory));
}

void Audio_Out_MQS::attach_interrupt(Context_Callback_Function<void> _user_cb, uint8_t priority) {
	//save the user callback function locally
	user_cb  = _user_cb;

	//update the IRQ_SOFTWARE vector with the new priority level 
	//and the static ISR as necessary
	attachInterruptVector(IRQ_SOFTWARE, user_callback_isr);
	NVIC_SET_PRIORITY(IRQ_SOFTWARE, priority);
	NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
}


//=============================================== PRIVATE UTILITY FUNCTIONS ===========================================

//configure clocking for MQS related functions
void Audio_Out_MQS::mqs_configure_clocks() {
	//in the clock controller module, enable clocking to 
	//the serial audio interface (SAI) peripheral and 
	//the medium quality sound (MQS) peripheral
	CCM_CCGR5 |= CCM_CCGR5_SAI3(CCM_CCGR_ON);
	CCM_CCGR0 |= CCM_CCGR0_MQS_HMCLK(CCM_CCGR_ON);

	/*
	 * Now we need to configure the Audio subsystem PLL
	 * essentially this runs right off the 24MHz input clock and boosts it up to a higher frequency
	 * These PLL constants are configured and verified in `config.hpp`
	 * 
	 * From the reference manual (p. 1028):
	 * 	PLL_output_frequency = F_ref * (DIV_SELECT + NUM/DENOM)
	 */
	set_audioClock(	Audio_Clocking_Constants::AUDIO_PLL_DIVSEL,
					Audio_Clocking_Constants::AUDIO_PLL_NUM,
					Audio_Clocking_Constants::AUDIO_PLL_DEN);


	/*
	 * Configuring the clock multiplexer going into the SAI3 peripheral
	 * SAI can derive its clock from a phase of PLL3, and from PLL4 or PLL5
	 * It also has some divider options that can be configured
	 */

	//configure SAI3 to source its clocking from PLL4 (Audio PLL), configured above
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI3_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI3_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4,

	//configure the pre-divider and post-dividier going into the SAI3 peripheral
	//scales the Audio PLL clock by a particular factor
	//PRED sets the "pre-divider", from 1-8
	//PODF sets the "post-divider" from 1-64
	//all of this set and validated in `config.hpp`
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI3_CLK_PRED_MASK | CCM_CS1CDR_SAI3_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI3_CLK_PRED(Audio_Clocking_Constants::SAI3_PRESC_1-1)
		   | CCM_CS1CDR_SAI3_CLK_PODF(Audio_Clocking_Constants::SAI3_PRESC_2-1);

	//might not be necessary, but sets up a particular MUX to output MCLK3 
	//specifically from the SPIDF0_CLK_ROOT
	//don't think we're using MCLK3 for anything though (in the chip or out of the chip) so this might just be a formality?
	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI3_MCLK3_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI3_MCLK_DIR | IOMUXC_GPR_GPR1_SAI3_MCLK3_SEL(0));	//Select MCLK

	/*
	 * Configuring basic operating parameters of MQS 
	 * 	Only thing to configure really has to deal with input clock divider and oversampling ratio compared to MCLK
	 *  --> oversampling setting is mostly vibes-based as to which of the two options sounds better
	 *  --> we're mostly handling clock division outside of the MQS peripheral, not adding additional division beyond this
	 * NOTE: MQS is typically clocked at 24.576MHz but can be clocked up to 66.5MHz
	 */

	//set the oversampling ratio and clock division of the MQS peripheral; enable it too
	//read oversampling configuration from `config.hpp`
	if(App_Constants::MQS_OVERSAMPLE_RATE == 64)
		IOMUXC_GPR_GPR2 = (IOMUXC_GPR_GPR2 & ~(IOMUXC_GPR_GPR2_MQS_OVERSAMPLE | IOMUXC_GPR_GPR2_MQS_CLK_DIV_MASK))
				| IOMUXC_GPR_GPR2_MQS_EN  | IOMUXC_GPR_GPR2_MQS_OVERSAMPLE | IOMUXC_GPR_GPR2_MQS_CLK_DIV(0);
	else //oversampling 32x
		IOMUXC_GPR_GPR2 = (IOMUXC_GPR_GPR2 & ~(IOMUXC_GPR_GPR2_MQS_OVERSAMPLE | IOMUXC_GPR_GPR2_MQS_CLK_DIV_MASK))
				| IOMUXC_GPR_GPR2_MQS_EN  | IOMUXC_GPR_GPR2_MQS_CLK_DIV(0);


	/*
	 * SAI3 peripheral configuration
	 * 	MQS is basically an internal I2S device that pulls data from the SAI3 peripheral
	 * 	As such, we need to configure SAI3 to do what we want	 
	 */

	//if for whatever reason everything has been configured (i.e. transmitter is already enabled)
	if (I2S3_TCSR & I2S_TCSR_TE) return;

	I2S3_TMR = 0; //All I2S transmit words are enabled (I think this has to deal with when you're sending a packet and want to ignore some bytes in the packet???)

	//set the threshold at which to fire a DMA request (in terms of how full the FIFO is)
	//basically fire the DMA request every time a word is consumed; only have one pending word in the FIFO at a time
	I2S3_TCR1 = I2S_TCR1_RFW(1);

	//run the transmitter asynchronously
	//clock from the SAI3_CLOCK_ROOT generated from the PLL; run this at 1/8 the speed of the SAI3 clock
	I2S3_TCR2 = I2S_TCR2_SYNC(0) /*| I2S_TCR2_BCP*/ // sync=0; tx is async;
		    | (I2S_TCR2_BCD | I2S_TCR2_DIV(((Audio_Clocking_Constants::I2S3_PRESC>>1) - 1)) | I2S_TCR2_MSEL(1));

	//each frame will have 1 word (32 bits) [CHANGED]
	//however, each half is only 16-bits, so we'll set our sync width to 16 bits (frame sync toggles every 16 bits)
	//MQS wants MSB shifted out first
	//and frame sync is generated internally (master mode)
	//frame sync polarity is ostensibly reversed due to the way memory is copied into the DMA buffer
	//as such the left channel is actually in the "right" position, so flip frame sync polarity
	I2S3_TCR4 = I2S_TCR4_FRSZ((1-1)) | I2S_TCR4_SYWD((16-1)) | I2S_TCR4_MF | I2S_TCR4_FSD /*| I2S_TCR4_FSE*/  | I2S_TCR4_FSP ;
	
	//set the word width to 32-bits (packing left and right channel into a single word) [CHANGED]
	//will allow slightly more efficient and intuitive memcpy's
	//As of now, data is packed (in the 32-bit word) as <MSB> [RIGHT][LEFT] <LSB> becuase of how `std::copy` works
	//as such we'll start transmitting our right channel first, then left
	//this is why we need to switch our frame sync polarity (see above)
	I2S3_TCR5 = I2S_TCR5_WNW((32-1)) | I2S_TCR5_W0W((32-1)) | I2S_TCR5_FBT((32-1));
	
	//enable the particular transmitter channel (channel 3)
	I2S3_TCR3 = I2S_TCR3_TCE;

	//and don't need to worry about receiver control, since we're just transmitting over MQS
}

//MQS DMA transfer half complete + complete
void Audio_Out_MQS::mqs_isr() {
	/*
	 * Determine which half of DMA memory to write to
	 * 	As far as I can tell, there isn't a flag that says we're servicing a `complete` or `half-complete` ISR
	 * 	so we'll do it the way `output_mqs.cpp` does it by looking at the current memory location being serviced by the DMA channel
	 * 	If it's servicing the first half of the buffer, we should write to the back half of the buffer, and vice-versa
	 */

	//get the current DMA address and the halfway point in our DMA memory chunk
	uint32_t dma_active_address = (uint32_t)(mqs_dma.sourceAddress());
	static const uint32_t dma_memory_midpoint = (uint32_t)(&dma_memory.half_buffers.backhalf);
	
	//if we're less than halfway through, we should write to the back half of memory
	//and if not, write to the front half
	if(dma_active_address < dma_memory_midpoint)
		dma_mem_write_to_fronthalf = false;
	else
		dma_mem_write_to_fronthalf = true;

	//clear the DMA interrupt status flag
	mqs_dma.clearInterrupt();

	//and run the user callback function at a reduced interrupt priority
	//generic software interrupt request 
	NVIC_SET_PENDING(IRQ_SOFTWARE);

	//ensure everything is synchronized--need this instruction in the ISR
    asm volatile("dsb");
}

void Audio_Out_MQS::user_callback_isr() {
	//clear the pending software interrupt request 
	//so we can immediately retrigger if necessary
	NVIC_CLEAR_PENDING(IRQ_SOFTWARE);

	//run the user callback function
	user_cb();

	//ensure everything is synchronized--need this instruction in the ISR
    asm volatile("dsb");
}



