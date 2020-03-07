#include "hsmci.h"
#include "check.h"
#include "core.h"
#include "board_serial.h"

void hsmci_write_protection_enable(const void* const hardware)
{
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_WPMR = (HSMCI_WPMR_WPKEY_PASSWD_Val << HSMCI_WPMR_WPKEY_Pos) | (1 << HSMCI_WPMR_WPEN_Pos);
	CRITICAL_SECTION_LEAVE()
}


void hsmci_write_protection_disable(const void* const hardware)
{
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_WPMR = (HSMCI_WPMR_WPKEY_PASSWD_Val << HSMCI_WPMR_WPKEY_Pos);
	CRITICAL_SECTION_LEAVE()
}


void hsmci_software_reset(const void* const hardware)
{
	CRITICAL_SECTION_ENTER();
	((Hsmci *)hardware)->HSMCI_CR = (1 << HSMCI_CR_SWRST_Pos);
	CRITICAL_SECTION_LEAVE();
}


void hsmci_soft_reset(const void* const hardware)
{
	// Save the state of all registers
	uint32_t mode_register = ((Hsmci *)hardware)->HSMCI_MR;
	uint32_t dtor_register = ((Hsmci *)hardware)->HSMCI_DTOR;
	uint32_t sdcr_register = ((Hsmci *)hardware)->HSMCI_SDCR;
	uint32_t cstor_register = ((Hsmci *)hardware)->HSMCI_CSTOR;
	uint32_t cfg_register = ((Hsmci *)hardware)->HSMCI_CFG;
	
	// Issue a software reset
	hsmci_software_reset(hardware);
	
	// Restore the previous state
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = mode_register;
	((Hsmci *)hardware)->HSMCI_DTOR = dtor_register;
	((Hsmci *)hardware)->HSMCI_SDCR = sdcr_register;
	((Hsmci *)hardware)->HSMCI_CSTOR = cstor_register;
	((Hsmci *)hardware)->HSMCI_CFG = cfg_register;
	CRITICAL_SECTION_LEAVE()
}


void hsmci_set_data_timeout(const void* const hardware, Hsmci_data_timeout_multiplier_e data_timeout_multiplier, uint8_t data_timeout_cycle_number)
{
	uint32_t tmp = (data_timeout_multiplier << HSMCI_DTOR_DTOMUL_Pos) | ((0b1111 & data_timeout_cycle_number) << HSMCI_DTOR_DTOCYC_Pos);
	
	CRITICAL_SECTION_ENTER();
	((Hsmci *)hardware)->HSMCI_DTOR = tmp;
	CRITICAL_SECTION_LEAVE();
}


void hsmci_set_completion_timeout(const void* const hardware, Hsmci_data_timeout_multiplier_e completion_signal_timout_multiplier, uint8_t completion_signal_timeout_cycle_number)
{
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_CSTOR = (completion_signal_timout_multiplier << HSMCI_CSTOR_CSTOMUL_Pos) | ((0xff & completion_signal_timeout_cycle_number) << HSMCI_CSTOR_CSTOCYC_Pos);
	CRITICAL_SECTION_LEAVE()
}


void hsmci_write_control_register(const void* const hardware, Hsmci_control_enable_e enable, Hsmci_control_powersave_e powersave)
{
	uint32_t tmp_reg = 0;
	
	if (enable == HSMCI_ENABLE)
	{
		tmp_reg |= (1 << HSMCI_CR_MCIEN_Pos);
	}
	else
	{
		tmp_reg |= (1 << HSMCI_CR_MCIDIS_Pos);
	}
	if (powersave == HSMCI_POWERSAVE_ON)
	{
		tmp_reg |= (1 << HSMCI_CR_PWSEN_Pos);
	}
	else
	{
		tmp_reg |= (1 << HSMCI_CR_PWSDIS_Pos);
	}
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_CR = tmp_reg;
	CRITICAL_SECTION_LEAVE()
}


void hsmci_sd_card_config(const void* const hardware, Hsmci_sd_bus_width_e bus_width, Hsmci_sd_slot_select_e slot_selct)
{
	uint32_t tmp = (bus_width << HSMCI_SDCR_SDCBUS_Pos) | (slot_selct << HSMCI_SDCR_SDCSEL_Pos);
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_SDCR = tmp;
	CRITICAL_SECTION_LEAVE()
}


void hsmci_set_block_length(const void* const hardware, uint32_t block_length, uint32_t block_count)
{
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_BLKR = ((0xffff & block_count) << HSMCI_BLKR_BCNT_Pos) | ((0xffff & block_length) << HSMCI_BLKR_BLKLEN_Pos);
	CRITICAL_SECTION_LEAVE()
}


void hsmci_write_argument_register(const void* const hardware, uint32_t argument)
{
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_ARGR = argument;
	CRITICAL_SECTION_LEAVE()
}


void hsmci_write_data_register(const void* const hardware, uint32_t data)
{
	while (!(hsmci_read_status_register(HSMCI) & (1 << HSMCI_SR_TXRDY_Pos)))
	{
		
	}
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_TDR = data;
	CRITICAL_SECTION_LEAVE()
}


uint32_t hsmci_read_data_register(const void* const hardware)
{
	// We have to wait for the RXRDY bit to be set in the status register
	while (!(hsmci_read_status_register(HSMCI) & (1 << HSMCI_SR_RXRDY_Pos)))
	{
		
	}
	
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_RDR;
	
	return tmp;
}


void hsmci_read_data_register_reverse(const void* const hardware, uint8_t* data, uint8_t number_of_words)
{
	check(number_of_words);
	
	uint32_t reg = 0;
	
	for (uint8_t i = 0; i < number_of_words; i++)
	{
		while (!(hsmci_read_status_register(HSMCI) & (1 << HSMCI_SR_RXRDY_Pos)))
		{
			
		}
		
		// Read the receive register
		reg = ((Hsmci *)hardware)->HSMCI_RDR;

		// Remap the bytes
		data[((4 * number_of_words) - 1) - 4*i]   = (reg >> 0) & 0xFF;
		data[((4 * number_of_words) - 1) - (4*i + 1)] = (reg >> 8) & 0xFF;
		data[((4 * number_of_words) - 1) - (4*i + 2)] = (reg >> 16) & 0xFF;
		data[((4 * number_of_words) - 1) - (4*i + 3)] = (reg >> 24) & 0xFF;
	}
}


uint32_t hsmci_read_48_bit_response_register(const void* const hardware)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_RSPR[0];
	
	return tmp;
}


void hsmci_read_136_bit_response_register_extended(const void* const hardware, uint8_t* response)
{
	// Note that the pointer is passed by reference
	uint32_t reg = 0;
	uint8_t byte_reverse[16];
	
	for (uint8_t i = 0; i < 4; i++)
	{
		reg = ((Hsmci *)hardware)->HSMCI_RSPR[i];

		byte_reverse[4*i]   = (reg >> 24) & 0xFF;
		byte_reverse[4*i + 1] = (reg >> 16) & 0xFF;
		byte_reverse[4*i + 2] = (reg >> 8) & 0xFF;
		byte_reverse[4*i + 3] = (reg >> 0) & 0xFF;
	}
	
	// Remap the bytes
	for (uint8_t i = 0; i < 16; i++)
	{
		*response = byte_reverse[15-i];
		response++;
	}
}


uint32_t hsmci_read_status_register(const void* const hardware)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_SR;
	
	return tmp;
}


Hsmci_status_e hsmci_send_command(const void* hardware, uint32_t command, uint32_t argument, Hsmci_check_crc_e crc)
{
	hsmci_force_byte_transfer_disable(hardware);
	hsmci_read_proof_disable(hardware);
	hsmci_write_proof_disable(hardware);
	
	HSMCI->HSMCI_DMA = 0;
	
	// Erase DMA and BLOCK register
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_DMA = 0;
	((Hsmci *)hardware)->HSMCI_BLKR = 0;
	CRITICAL_SECTION_LEAVE()
	
	// White the command and argument
	hsmci_write_argument_register(hardware, argument);
	hsmci_write_command_register(hardware, command);
	
	// Wait for the last command to be sent
	uint32_t status;
	
	do
	{
		status = hsmci_read_status_register(hardware);
		
	} while (!(status & (1 << HSMCI_SR_CMDRDY_Pos)));
	
	// Read the status register to check for errors
	if (status & HSMCI_STATUS_REGISTER_ERROR_MASK)
	{
		#if HSMCI_DEBUG
		// Print out error message including which command that caused the error
		// End print out the status register as well
		board_serial_print("[  FAIL ] CMD%d ", (HSMCI_CMDR_CMDNB_Msk & command));
		board_serial_print_register("status: ", status);
		#endif
		
		return HSMCI_ERROR;
	}
	
	if (status & (1 << HSMCI_SR_RCRCE_Pos))
	{
		// If CRC error, and we are supposed to check it
		if (crc == CHECK_CRC)
		{
			#if HSMCI_DEBUG
			// Print that it has occurred an CRC error
			board_serial_print("[WARNING] CRC error\n");
			#endif
			
			return HSMCI_ERROR;
		}
	}
	
	// If the response is R1b wait for the device to NOT return busy
	uint32_t timeout = 0xffffffff;
	if ((command & HSMCI_CMDR_RSPTYP_R1B) == HSMCI_CMDR_RSPTYP_R1B)
	{
		do
		{
			status = hsmci_read_status_register(hardware);
			if (timeout-- <= 1)
			{
				return HSMCI_ERROR;
			}
		} while (!((status & HSMCI_SR_NOTBUSY_Msk) && ((status & HSMCI_SR_DTOE_Msk) == 0)));
		
	}
	return HSMCI_OK;
}


void hsmci_write_command_register(const void* const hardware, uint32_t command)
{
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_CMDR = command;
	CRITICAL_SECTION_LEAVE()
}


Hsmci_status_e hsmci_send_addressed_data_transfer_command(	const void* const hardware, uint32_t command_register, uint32_t argument,
															uint16_t block_size, uint16_t number_of_blocks, Hsmci_check_crc_e crc)
{
	// Enable read and write proof
	hsmci_write_proof_enable(hardware);
	hsmci_read_proof_enable(hardware);
	
	HSMCI->HSMCI_DMA = 0;
	
	if (block_size & 0b11)
	{
		// Block size differ from modulo 2 and we have to enable force byte transfer
		hsmci_force_byte_transfer_enable(hardware);
	}
	else
	{
		hsmci_force_byte_transfer_disable(hardware);
	}
	
	// Check if we a doing a byte transfer
	if ((command_register & HSMCI_CMDR_TRTYP_BYTE) == HSMCI_CMDR_TRTYP_BYTE)
	{
		hsmci_set_block_length(hardware, 0, (block_size % 512));
	}
	else
	{
		hsmci_set_block_length(hardware, block_size, number_of_blocks);
	}
	
	// Erase DMA and BLOCK register
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_DMA = 0;
	CRITICAL_SECTION_LEAVE()
	
	// Write the argument and command register
	hsmci_write_argument_register(hardware, argument);
	hsmci_write_command_register(hardware, command_register);
	
	// Wait for the command to be sent
	uint32_t status;
	
	do
	{
		status = hsmci_read_status_register(hardware);
		
	} while (!(status & (1 << HSMCI_SR_CMDRDY_Pos)));
	
	// Check error flags	
	if (status & HSMCI_STATUS_REGISTER_ERROR_MASK)
	{
		#if HSMCI_DEBUG
		// Print out error message including which command that caused the error
		// End print out the status register as well
		board_serial_print("[  FAIL ] CMD%d ", (HSMCI_CMDR_CMDNB_Msk & command_register));
		board_serial_print_register("status: ", status);
		#endif
		
		return HSMCI_ERROR;
	}
	
	if (status & (1 << HSMCI_SR_RCRCE_Pos))
	{
		// If CRC error, and we are supposed to check it
		if (crc == CHECK_CRC)
		{
			#if HSMCI_DEBUG
			// Print that we have a CRC error
			board_serial_print("[  FAIL ] CMD%d ", (HSMCI_CMDR_CMDNB_Msk & command_register));
			board_serial_print("[WARNING] CRC error\n");
			#endif
			
			return HSMCI_ERROR;
		}
	}
	
	// If the response is R1b wait for the device to not return busy
	uint32_t timeout = 0;
	if ((command_register & HSMCI_CMDR_RSPTYP_R1B) == HSMCI_CMDR_RSPTYP_R1B)
	{
		do
		{
			status = hsmci_read_status_register(hardware);
			if (timeout++ == 0xffffffff)
			{
				return HSMCI_ERROR;
			}
		} while (!((status & HSMCI_SR_NOTBUSY_Msk) && ((status & HSMCI_SR_DTOE_Msk) == 0)));
		
	}
	
	return HSMCI_OK;
}


uint32_t hsmci_construct_command_register(	uint8_t boot_ack,
										uint8_t ata_with_command_completion_enable,
										Hsmci_command_sdio_special_command_e sdio_special_command,
										Hsmci_command_transfer_type_e transfer_type,
										Hsmci_command_transfer_direction_e transfer_direction,
										Hsmci_command_transfer_command_e transfer_command,
										Hsmci_command_max_latency_e max_latency,
										uint8_t open_drain_enable,
										Hsmci_command_special_command_e special_command,
										Hsmci_command_responce_type_e responce_type,
										uint8_t command_number)
{
	uint32_t tmp =	((0b1 & boot_ack) << HSMCI_CMDR_BOOT_ACK_Pos) |
					((0b1 & ata_with_command_completion_enable) << HSMCI_CMDR_ATACS_Pos) |
					(sdio_special_command << HSMCI_CMDR_IOSPCMD_Pos) |
					(transfer_type << HSMCI_CMDR_TRTYP_Pos) |
					(transfer_direction << HSMCI_CMDR_TRDIR_Pos) |
					(transfer_command << HSMCI_CMDR_TRCMD_Pos) |
					(max_latency << HSMCI_CMDR_MAXLAT_Pos) |
					((0b1 << open_drain_enable) << HSMCI_CMDR_OPDCMD_Pos) |
					(special_command << HSMCI_CMDR_SPCMD_Pos) |
					(responce_type << HSMCI_CMDR_RSPTYP_Pos) |
					((0b111111 << command_number) << HSMCI_CMDR_CMDNB_Pos);
	
	return tmp;
}


void hsmci_force_byte_transfer_enable(const void* const hardware)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_MR;
	
	tmp |= HSMCI_MR_FBYTE_Msk;
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE()
}

void hsmci_force_byte_transfer_disable(const void* const hardware)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_MR;
	
	tmp &= ~HSMCI_MR_FBYTE_Msk;
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE()
}

void hsmci_read_proof_enable(const void* const hardware)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_MR;
	
	tmp |= HSMCI_MR_RDPROOF_Msk;
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE()
}

void hsmci_read_proof_disable(const void* const hardware)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_MR;
	
	tmp &= ~HSMCI_MR_RDPROOF_Msk;
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE()
}

void hsmci_write_proof_enable(const void* const hardware)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_MR;
	
	tmp |= HSMCI_MR_WRPROOF_Msk;
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE()
}

void hsmci_write_proof_disable(const void* const hardware)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_MR;
	
	tmp &= ~HSMCI_MR_WRPROOF_Msk;
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE()
}

void hsmci_set_padding_value(const void* const hardware, Hsmci_mr_padding_e padding)
{
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_MR;
	
	if (padding == HSMCI_MR_PADDING_00)
	{
		tmp &= ~HSMCI_MR_PADV_Msk;
	}
	else 
	{
		tmp |= HSMCI_MR_PADV_Msk;
	}
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE()
}


void hsmci_set_bus_speed(const void* const hardware, uint32_t bus_speed, uint32_t cpu_peripheral_speed)
{
	uint8_t div_field;
	uint8_t odd_field;
	uint32_t prescaler;
	
	// Calculate the DIV field
	if (cpu_peripheral_speed > bus_speed * 2)
	{
		prescaler = cpu_peripheral_speed / bus_speed;
		prescaler -= 2;
		
		if (cpu_peripheral_speed % bus_speed)
		{
			prescaler++;
		}
		
		div_field = prescaler >> 1;
		odd_field = prescaler & 0b1;
	}
	else
	{
		div_field = 0;
		odd_field = 0;
	}
	
	uint32_t tmp = ((Hsmci *)hardware)->HSMCI_MR;
	
	tmp &= ~HSMCI_MR_CLKDIV_Msk;
	tmp &= ~HSMCI_MR_CLKODD_Msk;
	tmp |= (HSMCI_MR_CLKDIV_Msk & ((div_field + 1) << HSMCI_MR_CLKDIV_Pos));
	tmp |= (HSMCI_MR_CLKDIV_Msk & (odd_field << HSMCI_MR_CLKODD_Pos));
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE()
}


void hsmci_write_mode_register(	const void* const hardware,
								uint8_t odd_clock_divider,
								Hsmci_mr_padding_e padding,
								uint8_t force_byte_tranfer_enable,
								uint8_t write_proof_enable,
								uint8_t read_proof_enable,
								uint8_t power_save_divider,
								uint8_t clock_divider)
{
	uint32_t tmp =	((0b1 & odd_clock_divider) << HSMCI_MR_CLKODD_Pos) |
					((0b1 & padding) << HSMCI_MR_PADV_Pos) |
					((0b1 & force_byte_tranfer_enable) << HSMCI_MR_FBYTE_Pos) |
					((0b1 & write_proof_enable) << HSMCI_MR_WRPROOF_Pos) |
					((0b1 & read_proof_enable) << HSMCI_MR_RDPROOF_Pos) |
					((0b111 & power_save_divider) << HSMCI_MR_PWSDIV_Pos) |
					(clock_divider << HSMCI_MR_CLKDIV_Pos);
	
	CRITICAL_SECTION_ENTER();
	((Hsmci *)hardware)->HSMCI_MR = tmp;
	CRITICAL_SECTION_LEAVE();
}

void hsmci_high_speed_enable(const void* const hardware)
{
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_CFG |= (1 << HSMCI_CFG_HSMODE_Pos);
	CRITICAL_SECTION_LEAVE()
}


void hsmci_write_configuration_register(const void* const hardware,
										uint8_t synhronize_last_block,
										uint8_t high_speed_mode,
										uint8_t flow_error_reset_control_mode,
										uint8_t fifo_mode)
{
	uint32_t tmp =	((0b1 & synhronize_last_block) << HSMCI_CFG_LSYNC_Pos) |
					((0b1 & high_speed_mode) << HSMCI_CFG_HSMODE_Pos) |
					((0b1 & flow_error_reset_control_mode) << HSMCI_CFG_FERRCTRL_Pos) |
					((0b1 & fifo_mode) << HSMCI_CFG_FIFOMODE_Pos);
	
	CRITICAL_SECTION_ENTER()
	((Hsmci *)hardware)->HSMCI_CFG = tmp;
	CRITICAL_SECTION_LEAVE()
}