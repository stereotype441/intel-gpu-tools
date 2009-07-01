/*
 * Copyright © 2007,2009 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "intel_gpu_tools.h"
#include "instdone.h"

struct instdone_bit instdone_bits[MAX_INSTDONE_BITS];
int num_instdone_bits = 0;

static void
add_instdone_bit(uint32_t reg, uint32_t bit, const char *name)
{
	instdone_bits[num_instdone_bits].reg = reg;
	instdone_bits[num_instdone_bits].bit = bit;
	instdone_bits[num_instdone_bits].name = name;
	num_instdone_bits++;
}

static void
gen3_instdone_bit(uint32_t bit, const char *name)
{
	add_instdone_bit(INST_DONE, bit, name);
}

static void
gen4_instdone_bit(uint32_t bit, const char *name)
{
	add_instdone_bit(INST_DONE_I965, bit, name);
}

static void
gen4_instdone1_bit(uint32_t bit, const char *name)
{
	add_instdone_bit(INST_DONE_1, bit, name);
}

void
init_instdone_definitions(void)
{
	if (IS_965(devid)) {
		gen4_instdone_bit(I965_ROW_0_EU_0_DONE, "Row 0, EU 0");
		gen4_instdone_bit(I965_ROW_0_EU_1_DONE, "Row 0, EU 1");
		gen4_instdone_bit(I965_ROW_0_EU_2_DONE, "Row 0, EU 2");
		gen4_instdone_bit(I965_ROW_0_EU_3_DONE, "Row 0, EU 3");
		gen4_instdone_bit(I965_ROW_1_EU_0_DONE, "Row 1, EU 0");
		gen4_instdone_bit(I965_ROW_1_EU_1_DONE, "Row 1, EU 1");
		gen4_instdone_bit(I965_ROW_1_EU_2_DONE, "Row 1, EU 2");
		gen4_instdone_bit(I965_ROW_1_EU_3_DONE, "Row 1, EU 3");
		gen4_instdone_bit(I965_SF_DONE, "Strips and Fans");
		gen4_instdone_bit(I965_SE_DONE, "Setup Engine");
		gen4_instdone_bit(I965_WM_DONE, "Windowizer");
		gen4_instdone_bit(I965_DISPATCHER_DONE, "Dispatcher");
		gen4_instdone_bit(I965_PROJECTION_DONE, "Projection and LOD");
		gen4_instdone_bit(I965_DG_DONE, "Dependent address generator");
		gen4_instdone_bit(I965_QUAD_CACHE_DONE, "Texture fetch");
		gen4_instdone_bit(I965_TEXTURE_FETCH_DONE, "Texture fetch");
		gen4_instdone_bit(I965_TEXTURE_DECOMPRESS_DONE, "Texture decompress");
		gen4_instdone_bit(I965_SAMPLER_CACHE_DONE, "Sampler cache");
		gen4_instdone_bit(I965_FILTER_DONE, "Filtering");
		gen4_instdone_bit(I965_BYPASS_DONE, "Bypass FIFO");
		gen4_instdone_bit(I965_PS_DONE, "Pixel shader");
		gen4_instdone_bit(I965_CC_DONE, "Color calculator");
		gen4_instdone_bit(I965_MAP_FILTER_DONE, "Map filter");
		gen4_instdone_bit(I965_MAP_L2_IDLE, "Map L2");
		gen4_instdone_bit(I965_MA_ROW_0_DONE, "Message Arbiter row 0");
		gen4_instdone_bit(I965_MA_ROW_1_DONE, "Message Arbiter row 1");
		gen4_instdone_bit(I965_IC_ROW_0_DONE, "Instruction cache row 0");
		gen4_instdone_bit(I965_IC_ROW_1_DONE, "Instruction cache row 1");
		gen4_instdone_bit(I965_CP_DONE, "Command Processor");

		gen4_instdone1_bit(I965_GW_CS_DONE_CR, "GW CS CR");
		gen4_instdone1_bit(I965_SVSM_CS_DONE_CR, "SVSM CS CR");
		gen4_instdone1_bit(I965_SVDW_CS_DONE_CR, "SVDW CS CR");
		gen4_instdone1_bit(I965_SVDR_CS_DONE_CR, "SVDR CS CR");
		gen4_instdone1_bit(I965_SVRW_CS_DONE_CR, "SVRW CS CR");
		gen4_instdone1_bit(I965_SVRR_CS_DONE_CR, "SVRR CS CR");
		gen4_instdone1_bit(I965_SVTW_CS_DONE_CR, "SVTW CS CR");
		gen4_instdone1_bit(I965_MASM_CS_DONE_CR, "MASM CS CR");
		gen4_instdone1_bit(I965_MASF_CS_DONE_CR, "MASF CS CR");
		gen4_instdone1_bit(I965_MAW_CS_DONE_CR, "MAW CS CR");
		gen4_instdone1_bit(I965_EM1_CS_DONE_CR, "EM1 CS CR");
		gen4_instdone1_bit(I965_EM0_CS_DONE_CR, "EM0 CS CR");
		gen4_instdone1_bit(I965_UC1_CS_DONE, "UC1 CS");
		gen4_instdone1_bit(I965_UC0_CS_DONE, "UC0 CS");
		gen4_instdone1_bit(I965_URB_CS_DONE, "URB CS");
		gen4_instdone1_bit(I965_ISC_CS_DONE, "ISC CS");
		gen4_instdone1_bit(I965_CL_CS_DONE, "CL CS");
		gen4_instdone1_bit(I965_GS_CS_DONE, "GS CS");
		gen4_instdone1_bit(I965_VS0_CS_DONE, "VS0 CS");
		gen4_instdone1_bit(I965_VF_CS_DONE, "VF CS");
	} else if (IS_9XX(devid)) {
		gen3_instdone_bit(IDCT_DONE, "IDCT");
		gen3_instdone_bit(IQ_DONE, "IQ");
		gen3_instdone_bit(PR_DONE, "PR");
		gen3_instdone_bit(VLD_DONE, "VLD");
		gen3_instdone_bit(IP_DONE, "Instruction parser");
		gen3_instdone_bit(FBC_DONE, "Framebuffer Compression");
		gen3_instdone_bit(BINNER_DONE, "Binner");
		gen3_instdone_bit(SF_DONE, "Strips and fans");
		gen3_instdone_bit(SE_DONE, "Setup engine");
		gen3_instdone_bit(WM_DONE, "Windowizer");
		gen3_instdone_bit(IZ_DONE, "Intermediate Z");
		gen3_instdone_bit(PERSPECTIVE_INTERP_DONE, "Perspective interpolation");
		gen3_instdone_bit(DISPATCHER_DONE, "Dispatcher");
		gen3_instdone_bit(PROJECTION_DONE, "Projection and LOD");
		gen3_instdone_bit(DEPENDENT_ADDRESS_DONE, "Dependent address calculation");
		gen3_instdone_bit(TEXTURE_FETCH_DONE, "Texture fetch");
		gen3_instdone_bit(TEXTURE_DECOMPRESS_DONE, "Texture decompression");
		gen3_instdone_bit(SAMPLER_CACHE_DONE, "Sampler Cache");
		gen3_instdone_bit(FILTER_DONE, "Filtering");
		gen3_instdone_bit(BYPASS_FIFO_DONE, "Bypass FIFO");
		gen3_instdone_bit(PS_DONE, "Pixel shader");
		gen3_instdone_bit(CC_DONE, "Color calculator");
		gen3_instdone_bit(MAP_FILTER_DONE, "Map filter");
		gen3_instdone_bit(MAP_L2_IDLE, "Map L2");
	}
}