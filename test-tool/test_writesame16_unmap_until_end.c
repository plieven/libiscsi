/* 
   Copyright (C) 2013 Ronnie Sahlberg <ronniesahlberg@gmail.com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

#include <CUnit/CUnit.h>

#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-support.h"
#include "iscsi-test-cu.h"


void
test_writesame16_unmap_until_end(void)
{
	int ret;
	unsigned int i;
	unsigned char *buf = alloca(256 * block_size);

	CHECK_FOR_DATALOSS;
	CHECK_FOR_THIN_PROVISIONING;
	CHECK_FOR_LBPWS;
	CHECK_FOR_SBC;

	if (inq_bl->wsnz) {
	    logging(LOG_NORMAL, "[SKIPPED] WRITESAME10 does not support 0-blocks.");
	    CU_PASS("[SKIPPED] WRITESAME10 does not support 0-blocks.");
	    return;
	}

	logging(LOG_VERBOSE, LOG_BLANK_LINE);
	logging(LOG_VERBOSE, "Test WRITESAME16 of 1-256 blocks at the end of the LUN by setting number-of-blocks==0");
	memset(buf, 0xa6, 256 * block_size);
	for (i = 1; i <= 256; i++) {
		logging(LOG_VERBOSE, "Write %d blocks of 0xFF", i);
		memset(buf, 0xff, block_size * i);
		ret = write16(sd, num_blocks - i,
			      i * block_size, block_size, 0, 0, 0, 0, 0, buf,
			      EXPECT_STATUS_GOOD);
		logging(LOG_VERBOSE, "Unmap %d blocks using WRITESAME16", i);
		memset(buf, 0, block_size);
		ret = writesame16(sd, num_blocks - i,
				  block_size, 0, 0, 1, 0, 0, buf,
				  EXPECT_STATUS_GOOD);
		if (ret == -2) {
			logging(LOG_NORMAL, "[SKIPPED] WRITESAME16 is not implemented.");
			CU_PASS("[SKIPPED] Target does not support WRITESAME16. Skipping test");
			return;
		}
		CU_ASSERT_EQUAL(ret, 0);

		if (rc16->lbprz) {
			logging(LOG_VERBOSE, "LBPRZ is set. Read the unmapped "
				"blocks back and verify they are all zero");

			logging(LOG_VERBOSE, "Read %d blocks and verify they "
				"are now zero", i);
			ret = read16(sd, NULL, num_blocks - i,
				     i * block_size, block_size,
				     0, 0, 0, 0, 0, buf,
				     EXPECT_STATUS_GOOD);
			CU_ASSERT(all_zeroes(buf, i * block_size));
		} else {
			logging(LOG_VERBOSE, "LBPRZ is clear. Skip the read "
				"and verify zero test");
		}
	}
}
