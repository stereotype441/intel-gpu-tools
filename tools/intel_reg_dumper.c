/*
 * Copyright © 2006,2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include "intel_gpu_tools.h"

static uint32_t devid;

#define DEBUGSTRING(func) static void func(char *result, int len, int reg, uint32_t val)

DEBUGSTRING(i830_16bit_func)
{
	snprintf(result, len, "0x%04x", (uint16_t) val);
}

DEBUGSTRING(i830_debug_dcc)
{
	char *addressing = NULL;

	if (!IS_MOBILE(devid))
		return;

	if (IS_965(devid)) {
		if (val & (1 << 1))
			addressing = "dual channel interleaved";
		else
			addressing = "single or dual channel asymmetric";
	} else {
		switch (val & 3) {
		case 0:
			addressing = "single channel";
			break;
		case 1:
			addressing = "dual channel asymmetric";
			break;
		case 2:
			addressing = "dual channel interleaved";
			break;
		case 3:
			addressing = "unknown channel layout";
			break;
		}
	}

	snprintf(result, len, "%s, XOR randomization: %sabled, XOR bit: %d",
		 addressing,
		 (val & (1 << 10)) ? "dis" : "en",
		 (val & (1 << 9)) ? 17 : 11);
}

DEBUGSTRING(i830_debug_chdecmisc)
{
	char *enhmodesel = NULL;

	switch ((val >> 5) & 3) {
	case 1:
		enhmodesel = "XOR bank/rank";
		break;
	case 2:
		enhmodesel = "swap bank";
		break;
	case 3:
		enhmodesel = "XOR bank";
		break;
	case 0:
		enhmodesel = "none";
		break;
	}

	snprintf(result, len,
		 "%s, ch2 enh %sabled, ch1 enh %sabled, "
		 "ch0 enh %sabled, "
		 "flex %sabled, ep %spresent", enhmodesel,
		 (val & (1 << 4)) ? "en" : "dis",
		 (val & (1 << 3)) ? "en" : "dis",
		 (val & (1 << 2)) ? "en" : "dis",
		 (val & (1 << 1)) ? "en" : "dis",
		 (val & (1 << 0)) ? "" : "not ");
}

DEBUGSTRING(i830_debug_xyminus1)
{
	snprintf(result, len, "%d, %d", (val & 0xffff) + 1,
		 ((val & 0xffff0000) >> 16) + 1);
}

DEBUGSTRING(i830_debug_yxminus1)
{
	snprintf(result, len, "%d, %d", ((val & 0xffff0000) >> 16) + 1,
		 (val & 0xffff) + 1);
}

DEBUGSTRING(i830_debug_xy)
{
	snprintf(result, len, "%d, %d", (val & 0xffff), ((val & 0xffff0000) >> 16));
}

DEBUGSTRING(i830_debug_dspstride)
{
	snprintf(result, len, "%d bytes", val);
}

DEBUGSTRING(i830_debug_dspcntr)
{
	char *enabled = val & DISPLAY_PLANE_ENABLE ? "enabled" : "disabled";
	char plane = val & DISPPLANE_SEL_PIPE_B ? 'B' : 'A';
	if (HAS_PCH_SPLIT(devid))
		snprintf(result, len, "%s", enabled);
	else
		snprintf(result, len, "%s, pipe %c", enabled, plane);
}

DEBUGSTRING(i830_debug_pipeconf)
{
	char *enabled = val & PIPEACONF_ENABLE ? "enabled" : "disabled";
	char *bit30;

	if (IS_965(devid))
		bit30 = val & I965_PIPECONF_ACTIVE ? "active" : "inactive";
	else
		bit30 =
		    val & PIPEACONF_DOUBLE_WIDE ? "double-wide" : "single-wide";

	if (HAS_PCH_SPLIT(devid)) {
		char *bpc;

		switch (val & (7 << 5)) {
		case PIPECONF_8BPP:
			bpc = "8bpc";
			break;
		case PIPECONF_10BPP:
			bpc = "10bpc";
			break;
		case PIPECONF_6BPP:
			bpc = "6bpc";
			break;
		case PIPECONF_12BPP:
			bpc = "12bpc";
			break;
		default:
			bpc = "invalid bpc";
			break;
		}
		snprintf(result, len, "%s, %s, %s", enabled, bit30, bpc);
	} else
		snprintf(result, len, "%s, %s", enabled, bit30);
}

DEBUGSTRING(i830_debug_pipestat)
{
	char *_FIFO_UNDERRUN = val & FIFO_UNDERRUN ? " FIFO_UNDERRUN" : "";
	char *_CRC_ERROR_ENABLE =
	    val & CRC_ERROR_ENABLE ? " CRC_ERROR_ENABLE" : "";
	char *_CRC_DONE_ENABLE =
	    val & CRC_DONE_ENABLE ? " CRC_DONE_ENABLE" : "";
	char *_GMBUS_EVENT_ENABLE =
	    val & GMBUS_EVENT_ENABLE ? " GMBUS_EVENT_ENABLE" : "";
	char *_VSYNC_INT_ENABLE =
	    val & VSYNC_INT_ENABLE ? " VSYNC_INT_ENABLE" : "";
	char *_DLINE_COMPARE_ENABLE =
	    val & DLINE_COMPARE_ENABLE ? " DLINE_COMPARE_ENABLE" : "";
	char *_DPST_EVENT_ENABLE =
	    val & DPST_EVENT_ENABLE ? " DPST_EVENT_ENABLE" : "";
	char *_LBLC_EVENT_ENABLE =
	    val & LBLC_EVENT_ENABLE ? " LBLC_EVENT_ENABLE" : "";
	char *_OFIELD_INT_ENABLE =
	    val & OFIELD_INT_ENABLE ? " OFIELD_INT_ENABLE" : "";
	char *_EFIELD_INT_ENABLE =
	    val & EFIELD_INT_ENABLE ? " EFIELD_INT_ENABLE" : "";
	char *_SVBLANK_INT_ENABLE =
	    val & SVBLANK_INT_ENABLE ? " SVBLANK_INT_ENABLE" : "";
	char *_VBLANK_INT_ENABLE =
	    val & VBLANK_INT_ENABLE ? " VBLANK_INT_ENABLE" : "";
	char *_OREG_UPDATE_ENABLE =
	    val & OREG_UPDATE_ENABLE ? " OREG_UPDATE_ENABLE" : "";
	char *_CRC_ERROR_INT_STATUS =
	    val & CRC_ERROR_INT_STATUS ? " CRC_ERROR_INT_STATUS" : "";
	char *_CRC_DONE_INT_STATUS =
	    val & CRC_DONE_INT_STATUS ? " CRC_DONE_INT_STATUS" : "";
	char *_GMBUS_INT_STATUS =
	    val & GMBUS_INT_STATUS ? " GMBUS_INT_STATUS" : "";
	char *_VSYNC_INT_STATUS =
	    val & VSYNC_INT_STATUS ? " VSYNC_INT_STATUS" : "";
	char *_DLINE_COMPARE_STATUS =
	    val & DLINE_COMPARE_STATUS ? " DLINE_COMPARE_STATUS" : "";
	char *_DPST_EVENT_STATUS =
	    val & DPST_EVENT_STATUS ? " DPST_EVENT_STATUS" : "";
	char *_LBLC_EVENT_STATUS =
	    val & LBLC_EVENT_STATUS ? " LBLC_EVENT_STATUS" : "";
	char *_OFIELD_INT_STATUS =
	    val & OFIELD_INT_STATUS ? " OFIELD_INT_STATUS" : "";
	char *_EFIELD_INT_STATUS =
	    val & EFIELD_INT_STATUS ? " EFIELD_INT_STATUS" : "";
	char *_SVBLANK_INT_STATUS =
	    val & SVBLANK_INT_STATUS ? " SVBLANK_INT_STATUS" : "";
	char *_VBLANK_INT_STATUS =
	    val & VBLANK_INT_STATUS ? " VBLANK_INT_STATUS" : "";
	char *_OREG_UPDATE_STATUS =
	    val & OREG_UPDATE_STATUS ? " OREG_UPDATE_STATUS" : "";
	snprintf(result, len,
		 "status:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		 _FIFO_UNDERRUN,
		 _CRC_ERROR_ENABLE,
		 _CRC_DONE_ENABLE,
		 _GMBUS_EVENT_ENABLE,
		 _VSYNC_INT_ENABLE,
		 _DLINE_COMPARE_ENABLE,
		 _DPST_EVENT_ENABLE,
		 _LBLC_EVENT_ENABLE,
		 _OFIELD_INT_ENABLE,
		 _EFIELD_INT_ENABLE,
		 _SVBLANK_INT_ENABLE,
		 _VBLANK_INT_ENABLE,
		 _OREG_UPDATE_ENABLE,
		 _CRC_ERROR_INT_STATUS,
		 _CRC_DONE_INT_STATUS,
		 _GMBUS_INT_STATUS,
		 _VSYNC_INT_STATUS,
		 _DLINE_COMPARE_STATUS,
		 _DPST_EVENT_STATUS,
		 _LBLC_EVENT_STATUS,
		 _OFIELD_INT_STATUS,
		 _EFIELD_INT_STATUS,
		 _SVBLANK_INT_STATUS,
		 _VBLANK_INT_STATUS,
		 _OREG_UPDATE_STATUS);
}

DEBUGSTRING(i830_debug_hvtotal)
{
	snprintf(result, len, "%d active, %d total",
		 (val & 0xffff) + 1,
		 ((val & 0xffff0000) >> 16) + 1);
}

DEBUGSTRING(i830_debug_hvsyncblank)
{
	snprintf(result, len, "%d start, %d end",
		 (val & 0xffff) + 1,
		 ((val & 0xffff0000) >> 16) + 1);
}

DEBUGSTRING(i830_debug_vgacntrl)
{
	snprintf(result, len, "%s",
		 val & VGA_DISP_DISABLE ? "disabled" : "enabled");
}

DEBUGSTRING(i830_debug_fp)
{
	if (IS_IGD(devid)) {
		snprintf(result, len, "n = %d, m1 = %d, m2 = %d",
			 ffs((val & FP_N_IGD_DIV_MASK) >>
			     FP_N_DIV_SHIFT) - 1,
			 ((val & FP_M1_DIV_MASK) >> FP_M1_DIV_SHIFT),
			 ((val & FP_M2_IGD_DIV_MASK) >>
			  FP_M2_DIV_SHIFT));
	}
	snprintf(result, len, "n = %d, m1 = %d, m2 = %d",
		 ((val & FP_N_DIV_MASK) >> FP_N_DIV_SHIFT),
		 ((val & FP_M1_DIV_MASK) >> FP_M1_DIV_SHIFT),
		 ((val & FP_M2_DIV_MASK) >> FP_M2_DIV_SHIFT));
}

DEBUGSTRING(i830_debug_vga_pd)
{
	int vga0_p1, vga0_p2, vga1_p1, vga1_p2;

	/* XXX: i9xx version */

	if (val & VGA0_PD_P1_DIV_2)
		vga0_p1 = 2;
	else
		vga0_p1 = ((val & VGA0_PD_P1_MASK) >> VGA0_PD_P1_SHIFT) + 2;
	vga0_p2 = (val & VGA0_PD_P2_DIV_4) ? 4 : 2;

	if (val & VGA1_PD_P1_DIV_2)
		vga1_p1 = 2;
	else
		vga1_p1 = ((val & VGA1_PD_P1_MASK) >> VGA1_PD_P1_SHIFT) + 2;
	vga1_p2 = (val & VGA1_PD_P2_DIV_4) ? 4 : 2;

	snprintf(result, len, "vga0 p1 = %d, p2 = %d, vga1 p1 = %d, p2 = %d",
			 vga0_p1, vga0_p2, vga1_p1, vga1_p2);
}

DEBUGSTRING(i830_debug_pp_status)
{
	char *status = val & PP_ON ? "on" : "off";
	char *ready = val & PP_READY ? "ready" : "not ready";
	char *seq = "unknown";

	switch (val & PP_SEQUENCE_MASK) {
	case PP_SEQUENCE_NONE:
		seq = "idle";
		break;
	case PP_SEQUENCE_ON:
		seq = "on";
		break;
	case PP_SEQUENCE_OFF:
		seq = "off";
		break;
	}

	snprintf(result, len, "%s, %s, sequencing %s", status, ready, seq);
}

DEBUGSTRING(i830_debug_pp_control)
{
	snprintf(result, len, "power target: %s",
			 val & POWER_TARGET_ON ? "on" : "off");
}

DEBUGSTRING(i830_debug_dpll)
{
	char *enabled = val & DPLL_VCO_ENABLE ? "enabled" : "disabled";
	char *dvomode = val & DPLL_DVO_HIGH_SPEED ? "dvo" : "non-dvo";
	char *vgamode = val & DPLL_VGA_MODE_DIS ? "" : ", VGA";
	char *mode = "unknown";
	char *clock = "unknown";
	char *fpextra = val & DISPLAY_RATE_SELECT_FPA1 ? ", using FPx1!" : "";
	char sdvoextra[20];
	int p1, p2 = 0;

	if (IS_GEN2(devid)) {
		char is_lvds = (INREG(LVDS) & LVDS_PORT_EN) && (reg == DPLL_B);

		if (is_lvds) {
			mode = "LVDS";
			p1 = ffs((val & DPLL_FPA01_P1_POST_DIV_MASK_I830_LVDS)
				 >> DPLL_FPA01_P1_POST_DIV_SHIFT);
			if ((INREG(LVDS) & LVDS_CLKB_POWER_MASK) ==
			    LVDS_CLKB_POWER_UP)
				p2 = 7;
			else
				p2 = 14;

		} else {
			mode = "DAC/serial";
			if (val & PLL_P1_DIVIDE_BY_TWO) {
				p1 = 2;
			} else {
				/* Map the number in the field to (3, 33) */
				p1 = ((val & DPLL_FPA01_P1_POST_DIV_MASK_I830)
				      >> DPLL_FPA01_P1_POST_DIV_SHIFT) + 2;
			}
			if (val & PLL_P2_DIVIDE_BY_4)
				p2 = 4;
			else
				p2 = 2;
		}
	} else {
		if (IS_IGD(devid)) {
			p1 = ffs((val & DPLL_FPA01_P1_POST_DIV_MASK_IGD) >>
				 DPLL_FPA01_P1_POST_DIV_SHIFT_IGD);
		} else {
			p1 = ffs((val & DPLL_FPA01_P1_POST_DIV_MASK) >>
				 DPLL_FPA01_P1_POST_DIV_SHIFT);
		}
		switch (val & DPLL_MODE_MASK) {
		case DPLLB_MODE_DAC_SERIAL:
			mode = "DAC/serial";
			p2 = val & DPLL_DAC_SERIAL_P2_CLOCK_DIV_5 ? 5 : 10;
			break;
		case DPLLB_MODE_LVDS:
			mode = "LVDS";
			p2 = val & DPLLB_LVDS_P2_CLOCK_DIV_7 ? 7 : 14;
			break;
		}
	}

	switch (val & PLL_REF_INPUT_MASK) {
	case PLL_REF_INPUT_DREFCLK:
		clock = "default";
		break;
	case PLL_REF_INPUT_TVCLKINA:
		clock = "TV A";
		break;
	case PLL_REF_INPUT_TVCLKINBC:
		clock = "TV B/C";
		break;
	case PLLB_REF_INPUT_SPREADSPECTRUMIN:
		if (reg == DPLL_B)
			clock = "spread spectrum";
		break;
	}

	if (IS_945(devid)) {
		sprintf(sdvoextra, ", SDVO mult %d",
			(int)((val & SDVO_MULTIPLIER_MASK) >>
			      SDVO_MULTIPLIER_SHIFT_HIRES) + 1);
	} else {
		sdvoextra[0] = '\0';
	}

	snprintf(result, len, "%s, %s%s, %s clock, %s mode, p1 = %d, "
			 "p2 = %d%s%s",
			 enabled, dvomode, vgamode, clock, mode, p1, p2,
			 fpextra, sdvoextra);
}

DEBUGSTRING(i830_debug_dpll_test)
{
	char *dpllandiv = val & DPLLA_TEST_N_BYPASS ? ", DPLLA N bypassed" : "";
	char *dpllamdiv = val & DPLLA_TEST_M_BYPASS ? ", DPLLA M bypassed" : "";
	char *dpllainput = val & DPLLA_INPUT_BUFFER_ENABLE ?
	    "" : ", DPLLA input buffer disabled";
	char *dpllbndiv = val & DPLLB_TEST_N_BYPASS ? ", DPLLB N bypassed" : "";
	char *dpllbmdiv = val & DPLLB_TEST_M_BYPASS ? ", DPLLB M bypassed" : "";
	char *dpllbinput = val & DPLLB_INPUT_BUFFER_ENABLE ?
	    "" : ", DPLLB input buffer disabled";

	snprintf(result, len, "%s%s%s%s%s%s",
			 dpllandiv, dpllamdiv, dpllainput,
			 dpllbndiv, dpllbmdiv, dpllbinput);
}

DEBUGSTRING(i830_debug_adpa)
{
	char pipe = (val & ADPA_PIPE_B_SELECT) ? 'B' : 'A';
	char *enable = (val & ADPA_DAC_ENABLE) ? "enabled" : "disabled";
	char hsync = (val & ADPA_HSYNC_ACTIVE_HIGH) ? '+' : '-';
	char vsync = (val & ADPA_VSYNC_ACTIVE_HIGH) ? '+' : '-';

	if (HAS_CPT)
		pipe = val & (1<<29) ? 'B' : 'A';

	if (HAS_PCH_SPLIT(devid))
		snprintf(result, len, "%s, transcoder %c, %chsync, %cvsync",
				 enable, pipe, hsync, vsync);
	else
		snprintf(result, len, "%s, pipe %c, %chsync, %cvsync",
				 enable, pipe, hsync, vsync);
}

DEBUGSTRING(i830_debug_lvds)
{
	char pipe = val & LVDS_PIPEB_SELECT ? 'B' : 'A';
	char *enable = val & LVDS_PORT_EN ? "enabled" : "disabled";
	int depth;
	char *channels;

	if ((val & LVDS_A3_POWER_MASK) == LVDS_A3_POWER_UP)
		depth = 24;
	else
		depth = 18;
	if ((val & LVDS_B0B3_POWER_MASK) == LVDS_B0B3_POWER_UP)
		channels = "2 channels";
	else
		channels = "1 channel";

	if (HAS_CPT)
		pipe = val & (1<<29) ? 'B' : 'A';

	snprintf(result, len, "%s, pipe %c, %d bit, %s",
			 enable, pipe, depth, channels);
}

DEBUGSTRING(i830_debug_dvo)
{
	char *enable = val & DVO_ENABLE ? "enabled" : "disabled";
	char pipe = val & DVO_PIPE_B_SELECT ? 'B' : 'A';
	char *stall;
	char hsync = val & DVO_HSYNC_ACTIVE_HIGH ? '+' : '-';
	char vsync = val & DVO_VSYNC_ACTIVE_HIGH ? '+' : '-';

	switch (val & DVO_PIPE_STALL_MASK) {
	case DVO_PIPE_STALL_UNUSED:
		stall = "no stall";
		break;
	case DVO_PIPE_STALL:
		stall = "stall";
		break;
	case DVO_PIPE_STALL_TV:
		stall = "TV stall";
		break;
	default:
		stall = "unknown stall";
		break;
	}

	snprintf(result, len, "%s, pipe %c, %s, %chsync, %cvsync",
			 enable, pipe, stall, hsync, vsync);
}

DEBUGSTRING(i830_debug_sdvo)
{
	char *enable = val & SDVO_ENABLE ? "enabled" : "disabled";
	char pipe = val & SDVO_PIPE_B_SELECT ? 'B' : 'A';
	char *stall = val & SDVO_STALL_SELECT ? "enabled" : "disabled";
	char *detected = val & SDVO_DETECTED ? "" : "not ";
	char *gang = val & SDVOC_GANG_MODE ? ", gang mode" : "";
	char sdvoextra[20];

	if (IS_915(devid)) {
		sprintf(sdvoextra, ", SDVO mult %d",
			(int)((val & SDVO_PORT_MULTIPLY_MASK) >>
			      SDVO_PORT_MULTIPLY_SHIFT) + 1);
	} else {
		sdvoextra[0] = '\0';
	}

	snprintf(result, len, "%s, pipe %c, stall %s, %sdetected%s%s",
			 enable, pipe, stall, detected, sdvoextra, gang);
}

DEBUGSTRING(i830_debug_dspclk_gate_d)
{
	char *DPUNIT_B = val & DPUNIT_B_CLOCK_GATE_DISABLE ? " DPUNIT_B" : "";
	char *VSUNIT = val & VSUNIT_CLOCK_GATE_DISABLE ? " VSUNIT" : "";
	char *VRHUNIT = val & VRHUNIT_CLOCK_GATE_DISABLE ? " VRHUNIT" : "";
	char *VRDUNIT = val & VRDUNIT_CLOCK_GATE_DISABLE ? " VRDUNIT" : "";
	char *AUDUNIT = val & AUDUNIT_CLOCK_GATE_DISABLE ? " AUDUNIT" : "";
	char *DPUNIT_A = val & DPUNIT_A_CLOCK_GATE_DISABLE ? " DPUNIT_A" : "";
	char *DPCUNIT = val & DPCUNIT_CLOCK_GATE_DISABLE ? " DPCUNIT" : "";
	char *TVRUNIT = val & TVRUNIT_CLOCK_GATE_DISABLE ? " TVRUNIT" : "";
	char *TVCUNIT = val & TVCUNIT_CLOCK_GATE_DISABLE ? " TVCUNIT" : "";
	char *TVFUNIT = val & TVFUNIT_CLOCK_GATE_DISABLE ? " TVFUNIT" : "";
	char *TVEUNIT = val & TVEUNIT_CLOCK_GATE_DISABLE ? " TVEUNIT" : "";
	char *DVSUNIT = val & DVSUNIT_CLOCK_GATE_DISABLE ? " DVSUNIT" : "";
	char *DSSUNIT = val & DSSUNIT_CLOCK_GATE_DISABLE ? " DSSUNIT" : "";
	char *DDBUNIT = val & DDBUNIT_CLOCK_GATE_DISABLE ? " DDBUNIT" : "";
	char *DPRUNIT = val & DPRUNIT_CLOCK_GATE_DISABLE ? " DPRUNIT" : "";
	char *DPFUNIT = val & DPFUNIT_CLOCK_GATE_DISABLE ? " DPFUNIT" : "";
	char *DPBMUNIT = val & DPBMUNIT_CLOCK_GATE_DISABLE ? " DPBMUNIT" : "";
	char *DPLSUNIT = val & DPLSUNIT_CLOCK_GATE_DISABLE ? " DPLSUNIT" : "";
	char *DPLUNIT = val & DPLUNIT_CLOCK_GATE_DISABLE ? " DPLUNIT" : "";
	char *DPOUNIT = val & DPOUNIT_CLOCK_GATE_DISABLE ? " DPOUNIT" : "";
	char *DPBUNIT = val & DPBUNIT_CLOCK_GATE_DISABLE ? " DPBUNIT" : "";
	char *DCUNIT = val & DCUNIT_CLOCK_GATE_DISABLE ? " DCUNIT" : "";
	char *DPUNIT = val & DPUNIT_CLOCK_GATE_DISABLE ? " DPUNIT" : "";
	char *VRUNIT = val & VRUNIT_CLOCK_GATE_DISABLE ? " VRUNIT" : "";
	char *OVHUNIT = val & OVHUNIT_CLOCK_GATE_DISABLE ? " OVHUNIT" : "";
	char *DPIOUNIT = val & DPIOUNIT_CLOCK_GATE_DISABLE ? " DPIOUNIT" : "";
	char *OVFUNIT = val & OVFUNIT_CLOCK_GATE_DISABLE ? " OVFUNIT" : "";
	char *OVBUNIT = val & OVBUNIT_CLOCK_GATE_DISABLE ? " OVBUNIT" : "";
	char *OVRUNIT = val & OVRUNIT_CLOCK_GATE_DISABLE ? " OVRUNIT" : "";
	char *OVCUNIT = val & OVCUNIT_CLOCK_GATE_DISABLE ? " OVCUNIT" : "";
	char *OVUUNIT = val & OVUUNIT_CLOCK_GATE_DISABLE ? " OVUUNIT" : "";
	char *OVLUNIT = val & OVLUNIT_CLOCK_GATE_DISABLE ? " OVLUNIT" : "";

	snprintf(result, len,
		 "clock gates disabled:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		 DPUNIT_B, VSUNIT, VRHUNIT, VRDUNIT, AUDUNIT, DPUNIT_A, DPCUNIT,
		 TVRUNIT, TVCUNIT, TVFUNIT, TVEUNIT, DVSUNIT, DSSUNIT, DDBUNIT,
		 DPRUNIT, DPFUNIT, DPBMUNIT, DPLSUNIT, DPLUNIT, DPOUNIT, DPBUNIT,
		 DCUNIT, DPUNIT, VRUNIT, OVHUNIT, DPIOUNIT, OVFUNIT, OVBUNIT,
		 OVRUNIT, OVCUNIT, OVUUNIT, OVLUNIT);
}

DEBUGSTRING(i810_debug_915_fence)
{
	char format = (val & 1 << 12) ? 'Y' : 'X';
	int pitch = 128 << ((val & 0x70) >> 4);
	unsigned int offset = val & 0x0ff00000;
	int size = (1024 * 1024) << ((val & 0x700) >> 8);

	if (IS_965(devid) || (IS_915(devid) && reg >= FENCE_NEW))
		return;

	if (format == 'X')
		pitch *= 4;
	if (val & 1) {
		snprintf(result, len, "enabled, %c tiled, %4d pitch, 0x%08x - 0x%08x (%dkb)",
			 format, pitch, offset, offset + size,
			 size / 1024);
	} else {
		snprintf(result, len, "disabled");
	}
}

DEBUGSTRING(i810_debug_965_fence_start)
{
	char *enable = (val & FENCE_VALID) ? " enabled" : "disabled";
	char format = (val & I965_FENCE_Y_MAJOR) ? 'Y' : 'X';
	int pitch = ((val & 0xffc) >> 2) * 128 + 128;
	unsigned int offset = val & 0xfffff000;

	if (!IS_965(devid))
		return;

	snprintf(result, len, "%s, %c tile walk, %4d pitch, 0x%08x start",
		 enable, format, pitch, offset);
}

DEBUGSTRING(i810_debug_965_fence_end)
{
	unsigned int end = val & 0xfffff000;

	if (!IS_965(devid))
		return;

	snprintf(result, len, "                                   0x%08x end", end);
}

#define DEFINEREG(reg) \
	{ reg, #reg, NULL, 0 }
#define DEFINEREG_16BIT(reg) \
	{ reg, #reg, i830_16bit_func, 0 }
#define DEFINEREG2(reg, func) \
	{ reg, #reg, func, 0 }

struct reg_debug {
	int reg;
	char *name;
	void (*debug_output) (char *result, int len, int reg, uint32_t val);
	uint32_t val;
};

static struct reg_debug intel_debug_regs[] = {
	DEFINEREG2(DCC, i830_debug_dcc),
	DEFINEREG2(CHDECMISC, i830_debug_chdecmisc),
	DEFINEREG_16BIT(C0DRB0),
	DEFINEREG_16BIT(C0DRB1),
	DEFINEREG_16BIT(C0DRB2),
	DEFINEREG_16BIT(C0DRB3),
	DEFINEREG_16BIT(C1DRB0),
	DEFINEREG_16BIT(C1DRB1),
	DEFINEREG_16BIT(C1DRB2),
	DEFINEREG_16BIT(C1DRB3),
	DEFINEREG_16BIT(C0DRA01),
	DEFINEREG_16BIT(C0DRA23),
	DEFINEREG_16BIT(C1DRA01),
	DEFINEREG_16BIT(C1DRA23),

	DEFINEREG(PGETBL_CTL),

	DEFINEREG2(VCLK_DIVISOR_VGA0, i830_debug_fp),
	DEFINEREG2(VCLK_DIVISOR_VGA1, i830_debug_fp),
	DEFINEREG2(VCLK_POST_DIV, i830_debug_vga_pd),
	DEFINEREG2(DPLL_TEST, i830_debug_dpll_test),
	DEFINEREG(CACHE_MODE_0),
	DEFINEREG(D_STATE),
	DEFINEREG2(DSPCLK_GATE_D, i830_debug_dspclk_gate_d),
	DEFINEREG(RENCLK_GATE_D1),
	DEFINEREG(RENCLK_GATE_D2),
/*  DEFINEREG(RAMCLK_GATE_D),	CRL only */
	DEFINEREG2(SDVOB, i830_debug_sdvo),
	DEFINEREG2(SDVOC, i830_debug_sdvo),
/*    DEFINEREG(UDIB_SVB_SHB_CODES), CRL only */
/*    DEFINEREG(UDIB_SHA_BLANK_CODES), CRL only */
	DEFINEREG(SDVOUDI),
	DEFINEREG(DSPARB),
	DEFINEREG(DSPFW1),
	DEFINEREG(DSPFW2),
	DEFINEREG(DSPFW3),

	DEFINEREG2(ADPA, i830_debug_adpa),
	DEFINEREG2(LVDS, i830_debug_lvds),
	DEFINEREG2(DVOA, i830_debug_dvo),
	DEFINEREG2(DVOB, i830_debug_dvo),
	DEFINEREG2(DVOC, i830_debug_dvo),
	DEFINEREG(DVOA_SRCDIM),
	DEFINEREG(DVOB_SRCDIM),
	DEFINEREG(DVOC_SRCDIM),

	DEFINEREG2(PP_CONTROL, i830_debug_pp_control),
	DEFINEREG2(PP_STATUS, i830_debug_pp_status),
	DEFINEREG(PP_ON_DELAYS),
	DEFINEREG(PP_OFF_DELAYS),
	DEFINEREG(PP_DIVISOR),
	DEFINEREG(PFIT_CONTROL),
	DEFINEREG(PFIT_PGM_RATIOS),
	DEFINEREG(PORT_HOTPLUG_EN),
	DEFINEREG(PORT_HOTPLUG_STAT),

	DEFINEREG2(DSPACNTR, i830_debug_dspcntr),
	DEFINEREG2(DSPASTRIDE, i830_debug_dspstride),
	DEFINEREG2(DSPAPOS, i830_debug_xy),
	DEFINEREG2(DSPASIZE, i830_debug_xyminus1),
	DEFINEREG(DSPABASE),
	DEFINEREG(DSPASURF),
	DEFINEREG(DSPATILEOFF),
	DEFINEREG2(PIPEACONF, i830_debug_pipeconf),
	DEFINEREG2(PIPEASRC, i830_debug_yxminus1),
	DEFINEREG2(PIPEASTAT, i830_debug_pipestat),
	DEFINEREG(PIPEA_GMCH_DATA_M),
	DEFINEREG(PIPEA_GMCH_DATA_N),
	DEFINEREG(PIPEA_DP_LINK_M),
	DEFINEREG(PIPEA_DP_LINK_N),
	DEFINEREG(CURSOR_A_BASE),
	DEFINEREG(CURSOR_A_CONTROL),
	DEFINEREG(CURSOR_A_POSITION),

	DEFINEREG2(FPA0, i830_debug_fp),
	DEFINEREG2(FPA1, i830_debug_fp),
	DEFINEREG2(DPLL_A, i830_debug_dpll),
	DEFINEREG(DPLL_A_MD),
	DEFINEREG2(HTOTAL_A, i830_debug_hvtotal),
	DEFINEREG2(HBLANK_A, i830_debug_hvsyncblank),
	DEFINEREG2(HSYNC_A, i830_debug_hvsyncblank),
	DEFINEREG2(VTOTAL_A, i830_debug_hvtotal),
	DEFINEREG2(VBLANK_A, i830_debug_hvsyncblank),
	DEFINEREG2(VSYNC_A, i830_debug_hvsyncblank),
	DEFINEREG(BCLRPAT_A),
	DEFINEREG(VSYNCSHIFT_A),

	DEFINEREG2(DSPBCNTR, i830_debug_dspcntr),
	DEFINEREG2(DSPBSTRIDE, i830_debug_dspstride),
	DEFINEREG2(DSPBPOS, i830_debug_xy),
	DEFINEREG2(DSPBSIZE, i830_debug_xyminus1),
	DEFINEREG(DSPBBASE),
	DEFINEREG(DSPBSURF),
	DEFINEREG(DSPBTILEOFF),
	DEFINEREG2(PIPEBCONF, i830_debug_pipeconf),
	DEFINEREG2(PIPEBSRC, i830_debug_yxminus1),
	DEFINEREG2(PIPEBSTAT, i830_debug_pipestat),
	DEFINEREG(PIPEB_GMCH_DATA_M),
	DEFINEREG(PIPEB_GMCH_DATA_N),
	DEFINEREG(PIPEB_DP_LINK_M),
	DEFINEREG(PIPEB_DP_LINK_N),
	DEFINEREG(CURSOR_B_BASE),
	DEFINEREG(CURSOR_B_CONTROL),
	DEFINEREG(CURSOR_B_POSITION),

	DEFINEREG2(FPB0, i830_debug_fp),
	DEFINEREG2(FPB1, i830_debug_fp),
	DEFINEREG2(DPLL_B, i830_debug_dpll),
	DEFINEREG(DPLL_B_MD),
	DEFINEREG2(HTOTAL_B, i830_debug_hvtotal),
	DEFINEREG2(HBLANK_B, i830_debug_hvsyncblank),
	DEFINEREG2(HSYNC_B, i830_debug_hvsyncblank),
	DEFINEREG2(VTOTAL_B, i830_debug_hvtotal),
	DEFINEREG2(VBLANK_B, i830_debug_hvsyncblank),
	DEFINEREG2(VSYNC_B, i830_debug_hvsyncblank),
	DEFINEREG(BCLRPAT_B),
	DEFINEREG(VSYNCSHIFT_B),

	DEFINEREG(VCLK_DIVISOR_VGA0),
	DEFINEREG(VCLK_DIVISOR_VGA1),
	DEFINEREG(VCLK_POST_DIV),
	DEFINEREG2(VGACNTRL, i830_debug_vgacntrl),

	DEFINEREG(TV_CTL),
	DEFINEREG(TV_DAC),
	DEFINEREG(TV_CSC_Y),
	DEFINEREG(TV_CSC_Y2),
	DEFINEREG(TV_CSC_U),
	DEFINEREG(TV_CSC_U2),
	DEFINEREG(TV_CSC_V),
	DEFINEREG(TV_CSC_V2),
	DEFINEREG(TV_CLR_KNOBS),
	DEFINEREG(TV_CLR_LEVEL),
	DEFINEREG(TV_H_CTL_1),
	DEFINEREG(TV_H_CTL_2),
	DEFINEREG(TV_H_CTL_3),
	DEFINEREG(TV_V_CTL_1),
	DEFINEREG(TV_V_CTL_2),
	DEFINEREG(TV_V_CTL_3),
	DEFINEREG(TV_V_CTL_4),
	DEFINEREG(TV_V_CTL_5),
	DEFINEREG(TV_V_CTL_6),
	DEFINEREG(TV_V_CTL_7),
	DEFINEREG(TV_SC_CTL_1),
	DEFINEREG(TV_SC_CTL_2),
	DEFINEREG(TV_SC_CTL_3),
	DEFINEREG(TV_WIN_POS),
	DEFINEREG(TV_WIN_SIZE),
	DEFINEREG(TV_FILTER_CTL_1),
	DEFINEREG(TV_FILTER_CTL_2),
	DEFINEREG(TV_FILTER_CTL_3),
	DEFINEREG(TV_CC_CONTROL),
	DEFINEREG(TV_CC_DATA),
	DEFINEREG(TV_H_LUMA_0),
	DEFINEREG(TV_H_LUMA_59),
	DEFINEREG(TV_H_CHROMA_0),
	DEFINEREG(TV_H_CHROMA_59),

	DEFINEREG(FBC_CFB_BASE),
	DEFINEREG(FBC_LL_BASE),
	DEFINEREG(FBC_CONTROL),
	DEFINEREG(FBC_COMMAND),
	DEFINEREG(FBC_STATUS),
	DEFINEREG(FBC_CONTROL2),
	DEFINEREG(FBC_FENCE_OFF),
	DEFINEREG(FBC_MOD_NUM),

	DEFINEREG(MI_MODE),
	/* DEFINEREG(MI_DISPLAY_POWER_DOWN), CRL only */
	DEFINEREG(MI_ARB_STATE),
	DEFINEREG(MI_RDRET_STATE),
	DEFINEREG(ECOSKPD),

	DEFINEREG(DP_B),
	DEFINEREG(DPB_AUX_CH_CTL),
	DEFINEREG(DPB_AUX_CH_DATA1),
	DEFINEREG(DPB_AUX_CH_DATA2),
	DEFINEREG(DPB_AUX_CH_DATA3),
	DEFINEREG(DPB_AUX_CH_DATA4),
	DEFINEREG(DPB_AUX_CH_DATA5),

	DEFINEREG(DP_C),
	DEFINEREG(DPC_AUX_CH_CTL),
	DEFINEREG(DPC_AUX_CH_DATA1),
	DEFINEREG(DPC_AUX_CH_DATA2),
	DEFINEREG(DPC_AUX_CH_DATA3),
	DEFINEREG(DPC_AUX_CH_DATA4),
	DEFINEREG(DPC_AUX_CH_DATA5),

	DEFINEREG(DP_D),
	DEFINEREG(DPD_AUX_CH_CTL),
	DEFINEREG(DPD_AUX_CH_DATA1),
	DEFINEREG(DPD_AUX_CH_DATA2),
	DEFINEREG(DPD_AUX_CH_DATA3),
	DEFINEREG(DPD_AUX_CH_DATA4),
	DEFINEREG(DPD_AUX_CH_DATA5),

	DEFINEREG(AUD_CONFIG),
	DEFINEREG(AUD_HDMIW_STATUS),
	DEFINEREG(AUD_CONV_CHCNT),
	DEFINEREG(VIDEO_DIP_CTL),
	DEFINEREG(AUD_PINW_CNTR),
	DEFINEREG(AUD_CNTL_ST),
	DEFINEREG(AUD_PIN_CAP),
	DEFINEREG(AUD_PINW_CAP),
	DEFINEREG(AUD_PINW_UNSOLRESP),
	DEFINEREG(AUD_OUT_DIG_CNVT),
	DEFINEREG(AUD_OUT_CWCAP),
	DEFINEREG(AUD_GRP_CAP),

#define DEFINEFENCE_915(i) \
	{ FENCE+i*4, "FENCE  " #i, i810_debug_915_fence, 0 }
#define DEFINEFENCE_945(i)						\
	{ FENCE_NEW+(i - 8) * 4, "FENCE  " #i, i810_debug_915_fence, 0 }

	DEFINEFENCE_915(0),
	DEFINEFENCE_915(1),
	DEFINEFENCE_915(2),
	DEFINEFENCE_915(3),
	DEFINEFENCE_915(4),
	DEFINEFENCE_915(5),
	DEFINEFENCE_915(6),
	DEFINEFENCE_915(7),
	DEFINEFENCE_945(8),
	DEFINEFENCE_945(9),
	DEFINEFENCE_945(10),
	DEFINEFENCE_945(11),
	DEFINEFENCE_945(12),
	DEFINEFENCE_945(13),
	DEFINEFENCE_945(14),
	DEFINEFENCE_945(15),

#define DEFINEFENCE_965(i) \
	{ FENCE_NEW+i*8, "FENCE START " #i, i810_debug_965_fence_start, 0 }, \
	{ FENCE_NEW+i*8+4, "FENCE END " #i, i810_debug_965_fence_end, 0 }

	DEFINEFENCE_965(0),
	DEFINEFENCE_965(1),
	DEFINEFENCE_965(2),
	DEFINEFENCE_965(3),
	DEFINEFENCE_965(4),
	DEFINEFENCE_965(5),
	DEFINEFENCE_965(6),
	DEFINEFENCE_965(7),
	DEFINEFENCE_965(8),
	DEFINEFENCE_965(9),
	DEFINEFENCE_965(10),
	DEFINEFENCE_965(11),
	DEFINEFENCE_965(12),
	DEFINEFENCE_965(13),
	DEFINEFENCE_965(14),
	DEFINEFENCE_965(15),

	DEFINEREG(INST_PM),
};

DEBUGSTRING(ironlake_debug_rr_hw_ctl)
{
	snprintf(result, len, "low %d, high %d", val & RR_HW_LOW_POWER_FRAMES_MASK,
		 (val & RR_HW_HIGH_POWER_FRAMES_MASK) >> 8);
}

DEBUGSTRING(ironlake_debug_m_tu)
{
	snprintf(result, len, "TU %d, val 0x%x %d", (val >> 25) + 1, val & 0xffffff,
		 val & 0xffffff);
}

DEBUGSTRING(ironlake_debug_n)
{
	snprintf(result, len, "val 0x%x %d", val & 0xffffff, val & 0xffffff);
}

DEBUGSTRING(ironlake_debug_fdi_tx_ctl)
{
	char *train = NULL, *voltage = NULL, *pre_emphasis = NULL, *portw =
	    NULL;

	switch (val & FDI_LINK_TRAIN_NONE) {
	case FDI_LINK_TRAIN_PATTERN_1:
		train = "pattern_1";
		break;
	case FDI_LINK_TRAIN_PATTERN_2:
		train = "pattern_2";
		break;
	case FDI_LINK_TRAIN_PATTERN_IDLE:
		train = "pattern_idle";
		break;
	case FDI_LINK_TRAIN_NONE:
		train = "not train";
		break;
	}

	if (HAS_CPT) {
		/* SNB B0 */
		switch (val & (0x3f << 22)) {
		case FDI_LINK_TRAIN_400MV_0DB_SNB_B:
			voltage = "0.4V";
			pre_emphasis = "0dB";
			break;
		case FDI_LINK_TRAIN_400MV_6DB_SNB_B:
			voltage = "0.4V";
			pre_emphasis = "6dB";
			break;
		case FDI_LINK_TRAIN_600MV_3_5DB_SNB_B:
			voltage = "0.6V";
			pre_emphasis = "3.5dB";
			break;
		case FDI_LINK_TRAIN_800MV_0DB_SNB_B:
			voltage = "0.8V";
			pre_emphasis = "0dB";
			break;
		}

	} else {

		switch (val & (7 << 25)) {
			case FDI_LINK_TRAIN_VOLTAGE_0_4V:
				voltage = "0.4V";
				break;
			case FDI_LINK_TRAIN_VOLTAGE_0_6V:
				voltage = "0.6V";
				break;
			case FDI_LINK_TRAIN_VOLTAGE_0_8V:
				voltage = "0.8V";
				break;
			case FDI_LINK_TRAIN_VOLTAGE_1_2V:
				voltage = "1.2V";
				break;
			default:
				voltage = "reserved";
		}

		switch (val & (7 << 22)) {
			case FDI_LINK_TRAIN_PRE_EMPHASIS_NONE:
				pre_emphasis = "none";
				break;
			case FDI_LINK_TRAIN_PRE_EMPHASIS_1_5X:
				pre_emphasis = "1.5x";
				break;
			case FDI_LINK_TRAIN_PRE_EMPHASIS_2X:
				pre_emphasis = "2x";
				break;
			case FDI_LINK_TRAIN_PRE_EMPHASIS_3X:
				pre_emphasis = "3x";
				break;
			default:
				pre_emphasis = "reserved";
		}

	}

	switch (val & (7 << 19)) {
	case FDI_DP_PORT_WIDTH_X1:
		portw = "X1";
		break;
	case FDI_DP_PORT_WIDTH_X2:
		portw = "X2";
		break;
	case FDI_DP_PORT_WIDTH_X3:
		portw = "X3";
		break;
	case FDI_DP_PORT_WIDTH_X4:
		portw = "X4";
		break;
	}

	snprintf(result, len, "%s, train pattern %s, voltage swing %s,"
		 "pre-emphasis %s, port width %s, enhanced framing %s, FDI PLL %s, scrambing %s, master mode %s",
		 val & FDI_TX_ENABLE ? "enable" : "disable",
		 train, voltage, pre_emphasis, portw,
		 val & FDI_TX_ENHANCE_FRAME_ENABLE ? "enable" :
		 "disable",
		 val & FDI_TX_PLL_ENABLE ? "enable" : "disable",
		 val & (1 << 7) ? "disable" : "enable",
		 val & (1 << 0) ? "enable" : "disable");
}

DEBUGSTRING(ironlake_debug_fdi_rx_ctl)
{
	char *train = NULL, *portw = NULL, *bpc = NULL;

	if (HAS_CPT) {
		switch (val & FDI_LINK_TRAIN_PATTERN_MASK_CPT) {
		case FDI_LINK_TRAIN_PATTERN_1_CPT:
			train = "pattern_1";
			break;
		case FDI_LINK_TRAIN_PATTERN_2_CPT:
			train = "pattern_2";
			break;
		case FDI_LINK_TRAIN_PATTERN_IDLE_CPT:
			train = "pattern_idle";
			break;
		case FDI_LINK_TRAIN_NORMAL_CPT:
			train = "not train";
			break;
		}
	} else {
		switch (val & FDI_LINK_TRAIN_NONE) {
		case FDI_LINK_TRAIN_PATTERN_1:
			train = "pattern_1";
			break;
		case FDI_LINK_TRAIN_PATTERN_2:
			train = "pattern_2";
			break;
		case FDI_LINK_TRAIN_PATTERN_IDLE:
			train = "pattern_idle";
			break;
		case FDI_LINK_TRAIN_NONE:
			train = "not train";
			break;
		}
	}

	switch (val & (7 << 19)) {
	case FDI_DP_PORT_WIDTH_X1:
		portw = "X1";
		break;
	case FDI_DP_PORT_WIDTH_X2:
		portw = "X2";
		break;
	case FDI_DP_PORT_WIDTH_X3:
		portw = "X3";
		break;
	case FDI_DP_PORT_WIDTH_X4:
		portw = "X4";
		break;
	}

	switch (val & (7 << 16)) {
	case FDI_8BPC:
		bpc = "8bpc";
		break;
	case FDI_10BPC:
		bpc = "10bpc";
		break;
	case FDI_6BPC:
		bpc = "6bpc";
		break;
	case FDI_12BPC:
		bpc = "12bpc";
		break;
	}

	snprintf(result, len, "%s, train pattern %s, port width %s, %s,"
		 "link_reverse_strap_overwrite %s, dmi_link_reverse %s, FDI PLL %s,"
		 "FS ecc %s, FE ecc %s, FS err report %s, FE err report %s,"
		 "scrambing %s, enhanced framing %s, %s",
		 val & FDI_RX_ENABLE ? "enable" : "disable",
		 train, portw, bpc,
		 val & FDI_LINK_REVERSE_OVERWRITE ? "yes" : "no",
		 val & FDI_DMI_LINK_REVERSE_MASK ? "yes" : "no",
		 val & FDI_RX_PLL_ENABLE ? "enable" : "disable",
		 val & FDI_FS_ERR_CORRECT_ENABLE ? "enable" : "disable",
		 val & FDI_FE_ERR_CORRECT_ENABLE ? "enable" : "disable",
		 val & FDI_FS_ERR_REPORT_ENABLE ? "enable" : "disable",
		 val & FDI_FE_ERR_REPORT_ENABLE ? "enable" : "disable",
		 val & (1 << 7) ? "disable" : "enable",
		 val & FDI_RX_ENHANCE_FRAME_ENABLE ? "enable" :
		 "disable", val & FDI_SEL_PCDCLK ? "PCDClk" : "RawClk");
}

DEBUGSTRING(ironlake_debug_dspstride)
{
	snprintf(result, len, "%d", val >> 6);
}

DEBUGSTRING(ironlake_debug_pch_dpll)
{
	char *enable = val & DPLL_VCO_ENABLE ? "enable" : "disable";
	char *highspeed = val & DPLL_DVO_HIGH_SPEED ? "yes" : "no";
	char *mode = NULL;
	char *p2 = NULL;
	int fpa0_p1, fpa1_p1;
	char *refclk = NULL;
	int sdvo_mul;

	if ((val & DPLLB_MODE_LVDS) == DPLLB_MODE_LVDS) {
		mode = "LVDS";
		if (val & DPLLB_LVDS_P2_CLOCK_DIV_7)
			p2 = "Div 7";
		else
			p2 = "Div 14";
	} else if ((val & DPLLB_MODE_LVDS) == DPLLB_MODE_DAC_SERIAL) {
		mode = "Non-LVDS";
		if (val & DPLL_DAC_SERIAL_P2_CLOCK_DIV_5)
			p2 = "Div 5";
		else
			p2 = "Div 10";
	}
	fpa0_p1 = ffs((val & DPLL_FPA01_P1_POST_DIV_MASK) >> 16);
	fpa1_p1 = ffs((val & DPLL_FPA1_P1_POST_DIV_MASK));

	switch (val & PLL_REF_INPUT_MASK) {
	case PLL_REF_INPUT_DREFCLK:
		refclk = "default 120Mhz";
		break;
	case PLL_REF_INPUT_SUPER_SSC:
		refclk = "SuperSSC 120Mhz";
		break;
	case PLL_REF_INPUT_TVCLKINBC:
		refclk = "SDVO TVClkIn";
		break;
	case PLLB_REF_INPUT_SPREADSPECTRUMIN:
		refclk = "SSC";
		break;
	case PLL_REF_INPUT_DMICLK:
		refclk = "DMI RefCLK";
		break;
	}

	sdvo_mul = ((val & PLL_REF_SDVO_HDMI_MULTIPLIER_MASK) >> 9) + 1;

	snprintf(result, len, "%s, sdvo high speed %s, mode %s, p2 %s, "
		 "FPA0 P1 %d, FPA1 P1 %d, refclk %s, sdvo/hdmi mul %d",
		 enable, highspeed, mode, p2, fpa0_p1, fpa1_p1, refclk,
		 sdvo_mul);
}

DEBUGSTRING(ironlake_debug_dref_ctl)
{
	char *cpu_source;
	char *ssc_source = val & DREF_SSC_SOURCE_ENABLE ? "enable" : "disable";
	char *nonspread_source =
	    val & DREF_NONSPREAD_SOURCE_ENABLE ? "enable" : "disable";
	char *superspread_source =
	    val & DREF_SUPERSPREAD_SOURCE_ENABLE ? "enable" : "disable";
	char *ssc4_mode =
	    val & DREF_SSC4_CENTERSPREAD ? "centerspread" : "downspread";
	char *ssc1 = val & DREF_SSC1_ENABLE ? "enable" : "disable";
	char *ssc4 = val & DREF_SSC4_ENABLE ? "enable" : "disable";

	switch (val & DREF_CPU_SOURCE_OUTPUT_NONSPREAD) {
	case DREF_CPU_SOURCE_OUTPUT_DISABLE:
		cpu_source = "disable";
		break;
	case DREF_CPU_SOURCE_OUTPUT_DOWNSPREAD:
		cpu_source = "downspread";
		break;
	case DREF_CPU_SOURCE_OUTPUT_NONSPREAD:
		cpu_source = "nonspread";
		break;
	default:
		cpu_source = "reserved";
	}
	snprintf(result, len, "cpu source %s, ssc_source %s, nonspread_source %s, "
		 "superspread_source %s, ssc4_mode %s, ssc1 %s, ssc4 %s",
		 cpu_source, ssc_source, nonspread_source,
		 superspread_source, ssc4_mode, ssc1, ssc4);
}

DEBUGSTRING(ironlake_debug_rawclk_freq)
{
	char *tp1 = NULL, *tp2 = NULL;

	switch (val & FDL_TP1_TIMER_MASK) {
	case 0:
		tp1 = "0.5us";
		break;
	case (1 << 12):
		tp1 = "1.0us";
		break;
	case (2 << 12):
		tp1 = "2.0us";
		break;
	case (3 << 12):
		tp1 = "4.0us";
		break;
	}
	switch (val & FDL_TP2_TIMER_MASK) {
	case 0:
		tp2 = "1.5us";
		break;
	case (1 << 10):
		tp2 = "3.0us";
		break;
	case (2 << 10):
		tp2 = "6.0us";
		break;
	case (3 << 10):
		tp2 = "12.0us";
		break;
	}
	snprintf(result, len, "FDL_TP1 timer %s, FDL_TP2 timer %s, freq %d",
		 tp1, tp2, val & RAWCLK_FREQ_MASK);

}

DEBUGSTRING(ironlake_debug_fdi_rx_misc)
{
	snprintf(result, len, "FDI Delay %d", val & ((1 << 13) - 1));
}

DEBUGSTRING(ironlake_debug_transconf)
{
	snprintf(result, len, "%s, %s",
		 val & TRANS_ENABLE ? "enable" : "disable",
		 val & TRANS_STATE_ENABLE ? "active" : "inactive");
}

DEBUGSTRING(ironlake_debug_panel_fitting)
{
	char *vadapt = NULL, *filter_sel = NULL;

	switch (val & (3 << 25)) {
	case 0:
		vadapt = "least";
		break;
	case (1 << 25):
		vadapt = "moderate";
		break;
	case (2 << 25):
		vadapt = "reserved";
		break;
	case (3 << 25):
		vadapt = "most";
		break;
	}

	switch (val & (3 << 23)) {
	case 0:
		filter_sel = "programmed";
		break;
	case (1 << 23):
		filter_sel = "hardcoded";
		break;
	case (2 << 23):
		filter_sel = "edge_enhance";
		break;
	case (3 << 23):
		filter_sel = "edge_soften";
		break;
	}

	snprintf(result, len,
		 "%s, auto_scale %s, auto_scale_cal %s, v_filter %s, vadapt %s, mode %s, filter_sel %s,"
		 "chroma pre-filter %s, vert3tap %s, v_inter_invert %s",
		 val & PF_ENABLE ? "enable" : "disable",
		 val & (1 << 30) ? "no" : "yes",
		 val & (1 << 29) ? "yes" : "no",
		 val & (1 << 28) ? "bypass" : "enable",
		 val & (1 << 27) ? "enable" : "disable",
		 vadapt,
		 filter_sel,
		 val & (1 << 22) ? "enable" : "disable",
		 val & (1 << 21) ? "force" : "auto",
		 val & (1 << 20) ? "field 0" : "field 1");
}

DEBUGSTRING(ironlake_debug_panel_fitting_2)
{
	snprintf(result, len,
		 "vscale %f",
		 val / (float) (1<<15));
}

DEBUGSTRING(ironlake_debug_panel_fitting_3)
{
	snprintf(result, len,
		 "vscale initial phase %f",
		 val / (float) (1<<15));
}

DEBUGSTRING(ironlake_debug_panel_fitting_4)
{
	snprintf(result, len,
		 "hscale %f",
		 val / (float) (1<<15));
}

DEBUGSTRING(ironlake_debug_pf_win)
{
	int a, b;

	a = (val >> 16) & 0x1fff;
	b = val & 0xfff;

	snprintf(result, len, "%d, %d", a, b);
}

DEBUGSTRING(ironlake_debug_hdmi)
{
	int pipe;
	char *enable, *bpc = NULL, *encoding;
	char *mode, *audio, *vsync, *hsync, *detect;

	if (val & PORT_ENABLE)
		enable = "enabled";
	else
		enable = "disabled";

	if (HAS_CPT)
		pipe = (val & (3<<29)) >> 29;
	else
		pipe = (val & TRANSCODER_B) >> 29;

	switch (val & (7 << 26)) {
	case COLOR_FORMAT_8bpc:
		bpc = "8bpc";
		break;
	case COLOR_FORMAT_12bpc:
		bpc = "12bpc";
		break;
	}

	if ((val & (3 << 10)) == TMDS_ENCODING)
		encoding = "TMDS";
	else
		encoding = "SDVO";

	if (val & (1 << 9))
		mode = "HDMI";
	else
		mode = "DVI";

	if (val & AUDIO_ENABLE)
		audio = "enabled";
	else
		audio = "disabled";

	if (val & VSYNC_ACTIVE_HIGH)
		vsync = "+vsync";
	else
		vsync = "-vsync";

	if (val & HSYNC_ACTIVE_HIGH)
		hsync = "+hsync";
	else
		hsync = "-hsync";

	if (val & PORT_DETECTED)
		detect = "detected";
	else
		detect = "non-detected";

	snprintf(result, len, "%s pipe %c %s %s %s audio %s %s %s %s",
		 enable, pipe + 'A', bpc, encoding, mode, audio, vsync, hsync, detect);
}

DEBUGSTRING(snb_debug_dpll_sel)
{
	char *transa, *transb;
	char *dplla = NULL, *dpllb = NULL;

	if (!HAS_CPT)
		return;

	if (val & TRANSA_DPLL_ENABLE) {
		transa = "enable";
		if (val & TRANSA_DPLLB_SEL)
			dplla = "B";
		else
			dplla = "A";
	} else
		transa = "disable";

	if (val & TRANSB_DPLL_ENABLE) {
		transb = "enable";
		if (val & TRANSB_DPLLB_SEL)
			dpllb = "B";
		else
			dpllb = "A";
	} else
		transb = "disable";

	snprintf(result, len, "TransA DPLL %s (DPLL %s), TransB DPLL %s (DPLL %s)",
		 transa, dplla, transb, dpllb);
}

DEBUGSTRING(snb_debug_trans_dp_ctl)
{
	char *enable, *port = NULL, *bpc = NULL, *vsync, *hsync;

	if (!HAS_CPT)
		return;

	if (val & TRANS_DP_OUTPUT_ENABLE)
		enable = "enable";
	else
		enable = "disable";

	switch (val & TRANS_DP_PORT_SEL_MASK) {
	case TRANS_DP_PORT_SEL_B:
		port = "B";
		break;
	case TRANS_DP_PORT_SEL_C:
		port = "C";
		break;
	case TRANS_DP_PORT_SEL_D:
		port = "D";
		break;
	default:
		port = "none";
		break;
	}

	switch (val & (7<<9)) {
	case TRANS_DP_8BPC:
		bpc = "8bpc";
		break;
	case TRANS_DP_10BPC:
		bpc = "10bpc";
		break;
	case TRANS_DP_6BPC:
		bpc = "6bpc";
		break;
	case TRANS_DP_12BPC:
		bpc = "12bpc";
		break;
	}

	if (val & TRANS_DP_VSYNC_ACTIVE_HIGH)
		vsync = "+vsync";
	else
		vsync = "-vsync";

	if (val & TRANS_DP_HSYNC_ACTIVE_HIGH)
		hsync = "+hsync";
	else
		hsync = "-hsync";

	snprintf(result, len, "%s port %s %s %s %s",
		 enable, port, bpc, vsync, hsync);
}

DEBUGSTRING(ilk_debug_pp_control)
{
	snprintf(result, len, "blacklight %s, %spower down on reset, panel %s",
		 (val & (1 << 2)) ? "enabled" : "disabled",
		 (val & (1 << 1)) ? "" : "do not ",
		 (val & (1 << 0)) ? "on" : "off");
}

static struct reg_debug ironlake_debug_regs[] = {
	DEFINEREG(PGETBL_CTL),
	DEFINEREG(GEN6_INSTDONE_1),
	DEFINEREG(GEN6_INSTDONE_2),
	DEFINEREG2(CPU_VGACNTRL, i830_debug_vgacntrl),
	DEFINEREG(DIGITAL_PORT_HOTPLUG_CNTRL),

	DEFINEREG2(RR_HW_CTL, ironlake_debug_rr_hw_ctl),

	DEFINEREG(FDI_PLL_BIOS_0),
	DEFINEREG(FDI_PLL_BIOS_1),
	DEFINEREG(FDI_PLL_BIOS_2),

	DEFINEREG(DISPLAY_PORT_PLL_BIOS_0),
	DEFINEREG(DISPLAY_PORT_PLL_BIOS_1),
	DEFINEREG(DISPLAY_PORT_PLL_BIOS_2),

	DEFINEREG(FDI_PLL_FREQ_CTL),

	DEFINEREG2(PIPEACONF, i830_debug_pipeconf),

	DEFINEREG2(HTOTAL_A, i830_debug_hvtotal),
	DEFINEREG2(HBLANK_A, i830_debug_hvsyncblank),
	DEFINEREG2(HSYNC_A, i830_debug_hvsyncblank),
	DEFINEREG2(VTOTAL_A, i830_debug_hvtotal),
	DEFINEREG2(VBLANK_A, i830_debug_hvsyncblank),
	DEFINEREG2(VSYNC_A, i830_debug_hvsyncblank),
	DEFINEREG(VSYNCSHIFT_A),
	DEFINEREG2(PIPEASRC, i830_debug_yxminus1),

	DEFINEREG2(PIPEA_DATA_M1, ironlake_debug_m_tu),
	DEFINEREG2(PIPEA_DATA_N1, ironlake_debug_n),
	DEFINEREG2(PIPEA_DATA_M2, ironlake_debug_m_tu),
	DEFINEREG2(PIPEA_DATA_N2, ironlake_debug_n),

	DEFINEREG2(PIPEA_LINK_M1, ironlake_debug_n),
	DEFINEREG2(PIPEA_LINK_N1, ironlake_debug_n),
	DEFINEREG2(PIPEA_LINK_M2, ironlake_debug_n),
	DEFINEREG2(PIPEA_LINK_N2, ironlake_debug_n),

	DEFINEREG2(DSPACNTR, i830_debug_dspcntr),
	DEFINEREG(DSPABASE),
	DEFINEREG2(DSPASTRIDE, ironlake_debug_dspstride),
	DEFINEREG(DSPASURF),
	DEFINEREG2(DSPATILEOFF, i830_debug_xy),

	DEFINEREG2(PIPEBCONF, i830_debug_pipeconf),

	DEFINEREG2(HTOTAL_B, i830_debug_hvtotal),
	DEFINEREG2(HBLANK_B, i830_debug_hvsyncblank),
	DEFINEREG2(HSYNC_B, i830_debug_hvsyncblank),
	DEFINEREG2(VTOTAL_B, i830_debug_hvtotal),
	DEFINEREG2(VBLANK_B, i830_debug_hvsyncblank),
	DEFINEREG2(VSYNC_B, i830_debug_hvsyncblank),
	DEFINEREG(VSYNCSHIFT_B),

	DEFINEREG2(DSPBCNTR, i830_debug_dspcntr),
	DEFINEREG(DSPBBASE),
	DEFINEREG2(DSPBSTRIDE, ironlake_debug_dspstride),
	DEFINEREG(DSPBSURF),
	DEFINEREG2(DSPBTILEOFF, i830_debug_xy),

	DEFINEREG2(PIPEBSRC, i830_debug_yxminus1),

	DEFINEREG2(PIPEB_DATA_M1, ironlake_debug_m_tu),
	DEFINEREG2(PIPEB_DATA_N1, ironlake_debug_n),
	DEFINEREG2(PIPEB_DATA_M2, ironlake_debug_m_tu),
	DEFINEREG2(PIPEB_DATA_N2, ironlake_debug_n),

	DEFINEREG2(PIPEB_LINK_M1, ironlake_debug_n),
	DEFINEREG2(PIPEB_LINK_N1, ironlake_debug_n),
	DEFINEREG2(PIPEB_LINK_M2, ironlake_debug_n),
	DEFINEREG2(PIPEB_LINK_N2, ironlake_debug_n),

	DEFINEREG2(PFA_CTL_1, ironlake_debug_panel_fitting),
	DEFINEREG2(PFA_CTL_2, ironlake_debug_panel_fitting_2),
	DEFINEREG2(PFA_CTL_3, ironlake_debug_panel_fitting_3),
	DEFINEREG2(PFA_CTL_4, ironlake_debug_panel_fitting_4),
	DEFINEREG2(PFA_WIN_POS, ironlake_debug_pf_win),
	DEFINEREG2(PFA_WIN_SIZE, ironlake_debug_pf_win),
	DEFINEREG2(PFB_CTL_1, ironlake_debug_panel_fitting),
	DEFINEREG2(PFB_CTL_2, ironlake_debug_panel_fitting_2),
	DEFINEREG2(PFB_CTL_3, ironlake_debug_panel_fitting_3),
	DEFINEREG2(PFB_CTL_4, ironlake_debug_panel_fitting_4),
	DEFINEREG2(PFB_WIN_POS, ironlake_debug_pf_win),
	DEFINEREG2(PFB_WIN_SIZE, ironlake_debug_pf_win),

	/* PCH */

	DEFINEREG2(PCH_DREF_CONTROL, ironlake_debug_dref_ctl),
	DEFINEREG2(PCH_RAWCLK_FREQ, ironlake_debug_rawclk_freq),
	DEFINEREG(PCH_DPLL_TMR_CFG),
	DEFINEREG(PCH_SSC4_PARMS),
	DEFINEREG(PCH_SSC4_AUX_PARMS),
	DEFINEREG2(PCH_DPLL_SEL, snb_debug_dpll_sel),
	DEFINEREG(PCH_DPLL_ANALOG_CTL),

	DEFINEREG2(PCH_DPLL_A, ironlake_debug_pch_dpll),
	DEFINEREG2(PCH_DPLL_B, ironlake_debug_pch_dpll),
	DEFINEREG2(PCH_FPA0, i830_debug_fp),
	DEFINEREG2(PCH_FPA1, i830_debug_fp),
	DEFINEREG2(PCH_FPB0, i830_debug_fp),
	DEFINEREG2(PCH_FPB1, i830_debug_fp),

	DEFINEREG2(TRANS_HTOTAL_A, i830_debug_hvtotal),
	DEFINEREG2(TRANS_HBLANK_A, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_HSYNC_A, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_VTOTAL_A, i830_debug_hvtotal),
	DEFINEREG2(TRANS_VBLANK_A, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_VSYNC_A, i830_debug_hvsyncblank),

	DEFINEREG2(TRANSA_DATA_M1, ironlake_debug_m_tu),
	DEFINEREG2(TRANSA_DATA_N1, ironlake_debug_n),
	DEFINEREG2(TRANSA_DATA_M2, ironlake_debug_m_tu),
	DEFINEREG2(TRANSA_DATA_N2, ironlake_debug_n),
	DEFINEREG2(TRANSA_DP_LINK_M1, ironlake_debug_n),
	DEFINEREG2(TRANSA_DP_LINK_N1, ironlake_debug_n),
	DEFINEREG2(TRANSA_DP_LINK_M2, ironlake_debug_n),
	DEFINEREG2(TRANSA_DP_LINK_N2, ironlake_debug_n),

	DEFINEREG2(TRANS_HTOTAL_B, i830_debug_hvtotal),
	DEFINEREG2(TRANS_HBLANK_B, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_HSYNC_B, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_VTOTAL_B, i830_debug_hvtotal),
	DEFINEREG2(TRANS_VBLANK_B, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_VSYNC_B, i830_debug_hvsyncblank),

	DEFINEREG2(TRANSB_DATA_M1, ironlake_debug_m_tu),
	DEFINEREG2(TRANSB_DATA_N1, ironlake_debug_n),
	DEFINEREG2(TRANSB_DATA_M2, ironlake_debug_m_tu),
	DEFINEREG2(TRANSB_DATA_N2, ironlake_debug_n),
	DEFINEREG2(TRANSB_DP_LINK_M1, ironlake_debug_n),
	DEFINEREG2(TRANSB_DP_LINK_N1, ironlake_debug_n),
	DEFINEREG2(TRANSB_DP_LINK_M2, ironlake_debug_n),
	DEFINEREG2(TRANSB_DP_LINK_N2, ironlake_debug_n),

	DEFINEREG2(TRANS_HTOTAL_C, i830_debug_hvtotal),
	DEFINEREG2(TRANS_HBLANK_C, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_HSYNC_C, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_VTOTAL_C, i830_debug_hvtotal),
	DEFINEREG2(TRANS_VBLANK_C, i830_debug_hvsyncblank),
	DEFINEREG2(TRANS_VSYNC_C, i830_debug_hvsyncblank),

	DEFINEREG2(TRANSC_DATA_M1, ironlake_debug_m_tu),
	DEFINEREG2(TRANSC_DATA_N1, ironlake_debug_n),
	DEFINEREG2(TRANSC_DATA_M2, ironlake_debug_m_tu),
	DEFINEREG2(TRANSC_DATA_N2, ironlake_debug_n),
	DEFINEREG2(TRANSC_DP_LINK_M1, ironlake_debug_n),
	DEFINEREG2(TRANSC_DP_LINK_N1, ironlake_debug_n),
	DEFINEREG2(TRANSC_DP_LINK_M2, ironlake_debug_n),
	DEFINEREG2(TRANSC_DP_LINK_N2, ironlake_debug_n),

	DEFINEREG2(TRANSACONF, ironlake_debug_transconf),
	DEFINEREG2(TRANSBCONF, ironlake_debug_transconf),
	DEFINEREG2(TRANSCCONF, ironlake_debug_transconf),

	DEFINEREG2(FDI_TXA_CTL, ironlake_debug_fdi_tx_ctl),
	DEFINEREG2(FDI_TXB_CTL, ironlake_debug_fdi_tx_ctl),
	DEFINEREG2(FDI_TXC_CTL, ironlake_debug_fdi_tx_ctl),
	DEFINEREG2(FDI_RXA_CTL, ironlake_debug_fdi_rx_ctl),
	DEFINEREG2(FDI_RXB_CTL, ironlake_debug_fdi_rx_ctl),
	DEFINEREG2(FDI_RXC_CTL, ironlake_debug_fdi_rx_ctl),

	DEFINEREG2(FDI_RXA_MISC, ironlake_debug_fdi_rx_misc),
	DEFINEREG2(FDI_RXB_MISC, ironlake_debug_fdi_rx_misc),
	DEFINEREG2(FDI_RXC_MISC, ironlake_debug_fdi_rx_misc),
	DEFINEREG(FDI_RXA_TUSIZE1),
	DEFINEREG(FDI_RXA_TUSIZE2),
	DEFINEREG(FDI_RXB_TUSIZE1),
	DEFINEREG(FDI_RXB_TUSIZE2),
	DEFINEREG(FDI_RXC_TUSIZE1),
	DEFINEREG(FDI_RXC_TUSIZE2),

	DEFINEREG(FDI_PLL_CTL_1),
	DEFINEREG(FDI_PLL_CTL_2),

	DEFINEREG(FDI_RXA_IIR),
	DEFINEREG(FDI_RXA_IMR),
	DEFINEREG(FDI_RXB_IIR),
	DEFINEREG(FDI_RXB_IMR),

	DEFINEREG2(PCH_ADPA, i830_debug_adpa),
	DEFINEREG2(HDMIB, ironlake_debug_hdmi),
	DEFINEREG2(HDMIC, ironlake_debug_hdmi),
	DEFINEREG2(HDMID, ironlake_debug_hdmi),
	DEFINEREG2(PCH_LVDS, i830_debug_lvds),
	DEFINEREG(CPU_eDP_A),
	DEFINEREG(PCH_DP_B),
	DEFINEREG(PCH_DP_C),
	DEFINEREG(PCH_DP_D),
	DEFINEREG2(TRANS_DP_CTL_A, snb_debug_trans_dp_ctl),
	DEFINEREG2(TRANS_DP_CTL_B, snb_debug_trans_dp_ctl),
	DEFINEREG2(TRANS_DP_CTL_C, snb_debug_trans_dp_ctl),

	DEFINEREG(BLC_PWM_CPU_CTL2),
	DEFINEREG(BLC_PWM_CPU_CTL),
	DEFINEREG(BLC_PWM_PCH_CTL1),
	DEFINEREG(BLC_PWM_PCH_CTL2),

	DEFINEREG2(PCH_PP_STATUS, i830_debug_pp_status),
	DEFINEREG2(PCH_PP_CONTROL, ilk_debug_pp_control),
	DEFINEREG(PCH_PP_ON_DELAYS),
	DEFINEREG(PCH_PP_OFF_DELAYS),
	DEFINEREG(PCH_PP_DIVISOR),
};

static struct reg_debug i945gm_mi_regs[] = {
	DEFINEREG(PGETBL_CTL),
	DEFINEREG(PGTBL_ER),
	DEFINEREG(EXCC),
	DEFINEREG(HWS_PGA),
	DEFINEREG(IPEIR),
	DEFINEREG(IPEHR),
	DEFINEREG(INST_DONE),
	DEFINEREG(NOPID),
	DEFINEREG(HWSTAM),
	DEFINEREG(SCPD0),
	DEFINEREG(IER),
	DEFINEREG(IIR),
	DEFINEREG(IMR),
	DEFINEREG(ISR),
	DEFINEREG(EIR),
	DEFINEREG(EMR),
	DEFINEREG(ESR),
	DEFINEREG(INST_PM),
	DEFINEREG(ECOSKPD),
};

static void
i945_dump_mi_regs(void)
{
	char debug[1024];
	int i;

	for (i = 0; i < ARRAY_SIZE(i945gm_mi_regs); i++) {
		uint32_t val = INREG(i945gm_mi_regs[i].reg);

		if (i945gm_mi_regs[i].debug_output != NULL) {
			i945gm_mi_regs[i].debug_output(debug, sizeof(debug),
						       i945gm_mi_regs
						       [i].reg,
						       val);
			printf("%30.30s: 0x%08x (%s)\n",
			       i945gm_mi_regs[i].name,
			       (unsigned int)val, debug);
		} else {
			printf("%30.30s: 0x%08x\n", i945gm_mi_regs[i].name,
			       (unsigned int)val);
		}
	}
}

static void
ironlake_dump_regs(void)
{
	char debug[1024];
	int i;

	for (i = 0; i < ARRAY_SIZE(ironlake_debug_regs); i++) {
		uint32_t val = INREG(ironlake_debug_regs[i].reg);

		if (ironlake_debug_regs[i].debug_output != NULL) {
			ironlake_debug_regs[i].debug_output(debug, sizeof(debug),
							    ironlake_debug_regs
							    [i].reg,
							    val);
			printf("%30.30s: 0x%08x (%s)\n",
			       ironlake_debug_regs[i].name,
			       (unsigned int)val, debug);
		} else {
			printf("%30.30s: 0x%08x\n", ironlake_debug_regs[i].name,
			       (unsigned int)val);
		}
	}
}

static void
intel_dump_regs(void)
{
	char debug[1024];
	int i;
	int fp, dpll;
	int pipe;
	int n, m1, m2, m, p1, p2;
	int ref;
	int dot;
	int phase;
#if 0
	int msr;
	int crt;
#endif

	for (i = 0; i < ARRAY_SIZE(intel_debug_regs); i++) {
		uint32_t val = INREG(intel_debug_regs[i].reg);

		if (intel_debug_regs[i].debug_output != NULL) {
			intel_debug_regs[i].debug_output(debug, sizeof(debug),
							 intel_debug_regs[i].reg,
							 val);
			printf("%20.20s: 0x%08x (%s)\n",
			       intel_debug_regs[i].name,
			       (unsigned int)val, debug);
		} else {
			printf("%20.20s: 0x%08x\n",
			       intel_debug_regs[i].name,
			       (unsigned int)val);
		}
	}
#if 0
	i830DumpIndexed(pScrn, "SR", 0x3c4, 0x3c5, 0, 7);
	msr = INREG8(0x3cc);
	printf("%20.20s: 0x%02x\n",
		   "MSR", (unsigned int)msr);

	i830DumpAR(pScrn);
	if (msr & 1)
		crt = 0x3d0;
	else
		crt = 0x3b0;
	i830DumpIndexed(pScrn, "CR", crt + 4, crt + 5, 0, 0x24);
#endif
	for (pipe = 0; pipe <= 1; pipe++) {
		fp = INREG(pipe == 0 ? FPA0 : FPB0);
		dpll = INREG(pipe == 0 ? DPLL_A : DPLL_B);
		if (IS_GEN2(devid)) {
			uint32_t lvds = INREG(LVDS);
			if (devid == PCI_CHIP_I855_GM &&
			    (lvds & LVDS_PORT_EN) &&
			    (lvds & LVDS_PIPEB_SELECT) == (pipe << 30)) {
				if ((lvds & LVDS_CLKB_POWER_MASK) ==
				    LVDS_CLKB_POWER_UP)
					p2 = 7;
				else
					p2 = 14;
				switch ((dpll >> 16) & 0x3f) {
				case 0x01:
					p1 = 1;
					break;
				case 0x02:
					p1 = 2;
					break;
				case 0x04:
					p1 = 3;
					break;
				case 0x08:
					p1 = 4;
					break;
				case 0x10:
					p1 = 5;
					break;
				case 0x20:
					p1 = 6;
					break;
				default:
					p1 = 1;
					printf("LVDS P1 0x%x invalid encoding\n",
					       (dpll >> 16) & 0x3f);
					break;
				}
			} else {
				if (dpll & (1 << 23))
					p2 = 4;
				else
					p2 = 2;
				if (dpll & PLL_P1_DIVIDE_BY_TWO)
					p1 = 2;
				else
					p1 = ((dpll >> 16) & 0x3f) + 2;
			}

			switch ((dpll >> 13) & 0x3) {
			case 0:
				ref = 48000;
				break;
			case 3:
				ref = 66000;
				break;
			default:
				ref = 0;
				printf("ref out of range\n");
				break;
			}
		} else {
			uint32_t lvds = INREG(LVDS);
			if ((lvds & LVDS_PORT_EN) &&
			    (lvds & LVDS_PIPEB_SELECT) == (pipe << 30)) {
				if ((lvds & LVDS_CLKB_POWER_MASK) ==
				    LVDS_CLKB_POWER_UP)
					p2 = 7;
				else
					p2 = 14;
			} else {
				switch ((dpll >> 24) & 0x3) {
				case 0:
					p2 = 10;
					break;
				case 1:
					p2 = 5;
					break;
				default:
					p2 = 1;
					printf("p2 out of range\n");
					break;
				}
			}
			if (IS_IGD(devid))
				i = (dpll >> DPLL_FPA01_P1_POST_DIV_SHIFT_IGD) &
				    0x1ff;
			else
				i = (dpll >> DPLL_FPA01_P1_POST_DIV_SHIFT) &
				    0xff;
			switch (i) {
			case 1:
				p1 = 1;
				break;
			case 2:
				p1 = 2;
				break;
			case 4:
				p1 = 3;
				break;
			case 8:
				p1 = 4;
				break;
			case 16:
				p1 = 5;
				break;
			case 32:
				p1 = 6;
				break;
			case 64:
				p1 = 7;
				break;
			case 128:
				p1 = 8;
				break;
			case 256:
				if (IS_IGD(devid)) {
					p1 = 9;
					break;
				}	/* fallback */
			default:
				p1 = 1;
				printf("p1 out of range\n");
				break;
			}

			switch ((dpll >> 13) & 0x3) {
			case 0:
				ref = 96000;
				break;
			case 3:
				ref = 100000;
				break;
			default:
				ref = 0;
				printf("ref out of range\n");
				break;
			}
		}
		if (IS_965(devid)) {
			phase = (dpll >> 9) & 0xf;
			switch (phase) {
			case 6:
				break;
			default:
				printf("SDVO phase shift %d out of range -- probobly not "
				       "an issue.\n", phase);
				break;
			}
		}
		switch ((dpll >> 8) & 1) {
		case 0:
			break;
		default:
			printf("fp select out of range\n");
			break;
		}
		m1 = ((fp >> 8) & 0x3f);
		if (IS_IGD(devid)) {
			n = ffs((fp & FP_N_IGD_DIV_MASK) >> FP_N_DIV_SHIFT) - 1;
			m2 = (fp & FP_M2_IGD_DIV_MASK) >> FP_M2_DIV_SHIFT;
			m = m2 + 2;
			dot = (ref * m) / n / (p1 * p2);
		} else {
			n = ((fp >> 16) & 0x3f);
			m2 = ((fp >> 0) & 0x3f);
			m = 5 * (m1 + 2) + (m2 + 2);
			dot =
			    (ref * (5 * (m1 + 2) + (m2 + 2)) / (n + 2)) / (p1 *
									   p2);
		}

		printf("pipe %s dot %d n %d m1 %d m2 %d p1 %d p2 %d\n",
		       pipe == 0 ? "A" : "B", dot, n, m1, m2, p1, p2);
	}
}

int main(int argc, char** argv)
{
	struct pci_device *pci_dev;

	if (argc == 2)
		intel_map_file(argv[1]);
	else {
		pci_dev = intel_get_pci_device();
		devid = pci_dev->device_id; /* XXX not true when mapping! */

		intel_get_mmio(pci_dev);
	}

	if (HAS_PCH_SPLIT(devid) || getenv("HAS_PCH_SPLIT")) {
		intel_check_pch();
		ironlake_dump_regs();
	}
	else if (IS_945GM(devid)) {
		i945_dump_mi_regs();
		intel_dump_regs();
	} else
		intel_dump_regs();

	return 0;
}
