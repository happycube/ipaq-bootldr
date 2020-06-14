#!/usr/bin/perl -w

require "POSIX.pm";

$model = "h5400";

$gafr_name = [ "GAFR0_L", "GAFR0_U", "GAFR1_L", "GAFR1_U", "GAFR2_L", "GAFR2_U" ]; 

$ALT_FCN_1 = 1;
$ALT_FCN_2 = 2;
$ALT_FCN_3 = 3;

$IN = 0;
$OUT = 1;

$SET = 1;
$CLEAR = 2;

$alt_fcn_info{"GPIO1_RTS_MD"} =			[ 1,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO6_MMCCLK_MD"} =		[ 6,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO8_48MHz_MD"} =		[ 8,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO8_MMCCS0_MD"} =		[ 8,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO9_MMCCS1_MD"} =		[ 9,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO10_RTCCLK_MD"} =		[10,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO11_3_6MHz_MD"} =		[11,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO12_32KHz_MD"} =		[12,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO13_MBGNT_MD"} =		[13,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO14_MBREQ_MD"} =		[14,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO15_nCS_1_MD"} =		[15,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO16_PWM0_MD"} =		[16,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO17_PWM1_MD"} =		[17,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO18_RDY_MD"} =		[18,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO19_DREQ1_MD"} =		[19,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO20_DREQ0_MD"} =		[20,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO23_SCLK_MD"} =		[23,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO24_SFRM_MD"} =		[24,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO25_STXD_MD"} =		[25,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO26_SRXD_MD"} =		[26,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO27_SEXTCLK_MD"} =		[27,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO28_BITCLK_AC97_MD"} =	[28,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO28_BITCLK_IN_I2S_MD"} =	[28,  $ALT_FCN_2, $IN,	0];
$alt_fcn_info{"GPIO28_BITCLK_OUT_I2S_MD"} = 	[28,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO29_SDATA_IN_AC97_MD"} =	[29,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO29_SDATA_IN_I2S_MD"} =	[29,  $ALT_FCN_2, $IN,	0];
$alt_fcn_info{"GPIO30_SDATA_OUT_AC97_MD"} =	[30,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO30_SDATA_OUT_I2S_MD"} =	[30,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO31_SYNC_AC97_MD"} =		[31,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO31_SYNC_I2S_MD"} =		[31,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO32_SDATA_IN1_AC97_MD"} =	[32,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO32_SYSCLK_I2S_MD"} =		[32,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO33_nCS_5_MD"} =		[33,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO34_FFRXD_MD"} =		[34,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO34_MMCCS0_MD"} =		[34,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO35_FFCTS_MD"} =		[35,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO36_FFDCD_MD"} =		[36,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO37_FFDSR_MD"} =		[37,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO38_FFRI_MD"} =		[38,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO39_MMCCS1_MD"} =		[39,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO39_FFTXD_MD"} =		[39,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO40_FFDTR_MD"} =		[40,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO41_FFRTS_MD"} =		[41,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO42_BTRXD_MD"} =		[42,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO43_BTTXD_MD"} =		[43,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO44_BTCTS_MD"} =		[44,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO45_BTRTS_MD"} =		[45,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO46_ICPRXD_MD"} =		[46,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO46_STRXD_MD"} =		[46,  $ALT_FCN_2, $IN,	0];
$alt_fcn_info{"GPIO47_ICPTXD_MD"} =		[47,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO47_STTXD_MD"} =		[47,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO48_POE_N_MD"} =		[48,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO49_PWE_N_MD"} =		[49,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO50_PIOR_N_MD"} =		[50,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO51_PIOW_N_MD"} =		[51,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO52_PCE1_N_MD"} =		[52,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO53_PCE2_N_MD"} =		[53,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO53_MMCCLK_MD"} =		[53,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO54_MMCCLK_MD"} =		[54,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO54_PSKTSEL_MD"} =		[54,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO55_PREG_N_MD"} =		[55,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO56_PWAIT_N_MD"} =		[56,  $ALT_FCN_1, $IN,	$SET];
$alt_fcn_info{"GPIO57_IOIS16_N_MD"} =		[57,  $ALT_FCN_1, $IN,	$SET];
$alt_fcn_info{"GPIO58_LDD_0_MD"} =		[58,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO59_LDD_1_MD"} =		[59,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO60_LDD_2_MD"} =		[60,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO61_LDD_3_MD"} =		[61,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO62_LDD_4_MD"} =		[62,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO63_LDD_5_MD"} =		[63,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO64_LDD_6_MD"} =		[64,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO65_LDD_7_MD"} =		[65,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO66_LDD_8_MD"} =		[66,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO66_MBREQ_MD"} =		[66,  $ALT_FCN_1, $IN,	0];
$alt_fcn_info{"GPIO67_LDD_9_MD"} =		[67,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO67_MMCCS0_MD"} =	[67,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO68_LDD_10_MD"} =	[68,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO68_MMCCS1_MD"} =	[68,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO69_LDD_11_MD"} =	[69,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO69_MMCCLK_MD"} =	[69,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO70_LDD_12_MD"} =	[70,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO70_RTCCLK_MD"} =	[70,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO71_LDD_13_MD"} =	[71,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO71_3_6MHz_MD"} =	[71,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO72_LDD_14_MD"} =	[72,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO72_32kHz_MD"} =		[72,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO73_LDD_15_MD"} =	[73,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO73_MBGNT_MD"} =		[73,  $ALT_FCN_1, $OUT,	0];
$alt_fcn_info{"GPIO74_LCD_FCLK_MD"} =	[74,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO75_LCD_LCLK_MD"} =	[75,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO76_LCD_PCLK_MD"} =	[76,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO77_LCD_ACBIAS_MD"} =	[77,  $ALT_FCN_2, $OUT,	0];
$alt_fcn_info{"GPIO78_nCS_2_MD"} =		[78,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO79_nCS_3_MD"} =		[79,  $ALT_FCN_2, $OUT,	$SET];
$alt_fcn_info{"GPIO80_nCS_4_MD"} =		[80,  $ALT_FCN_2, $OUT,	$SET];


sub gen_header {
    print HEADER "\n\n";
    while (<STDIN>) {
	if (m/gpio(\d+)\s+([_A-Za-z0-9]+)\s+([_A-Za-z0-9]+)\s*(\/\*.*\*\/)?\s*$/o) {
	    my ($nr, $name, $fcn, $comment) = ($1, $2, $3, $4);
	    my $gpio_nr_name = POSIX::toupper("gpio_nr_" . $model . "_" . $name);

	    $gpios[$nr] = $nr;
	    $names[$nr] = $name;
	    $fcns[$nr] = $fcn;
	    $gpio_nr_names[$nr] = $gpio_nr_name;
	    $comments[$nr] = $comment;
	    $sets[int($nr)] = 0;

	    print HEADER "#define $gpio_nr_name $nr";
	    print HEADER $comment if ($comment);
	    print HEADER "\n";
	    if ($fcn eq "nc" || $fcn eq "input") {
		$dirs[$nr] = 0;
	    } elsif ($fcn eq "output") {
		$dirs[$nr] = 1;
	    } elsif ($fcn eq "output_n") {
		$dirs[$nr] = 1;
		$sets[$nr] = 1;
	    } else {
		my $gpio_mode_name = &POSIX::toupper("gpio$nr" . "_" . $model . "_" . $name . "_mode");
		my $gpio_mode = &POSIX::toupper("gpio$nr" . "_" . $fcn . "_md");
		$_ = $fcn;
		if (m/cs(\d)_n/o) {
		    my $old = "CS" . $1 . "_N";
		    my $new = "nCS_$1";
		    $gpio_mode =~ s/$old/$new/;
		}
		$_ = $gpio_mode;
		$mode_line{$nr} = "#define $gpio_mode_name $gpio_mode\n";
		my $altfcnspec = $alt_fcn_info{$gpio_mode};
		if ($altfcnspec) {
		    $altfcn[int($nr)] = $$altfcnspec[1];
		    $dirs[$nr] = $$altfcnspec[2];
		    print $gpio_mode, "\t";
		    print sprintf(":gpio $nr altfcnspec: %d %x %x %x\n", 
				  $$altfcnspec[0], $$altfcnspec[1], $$altfcnspec[2], $$altfcnspec[3]);
		    $sets[int($nr)] = $$altfcnspec[3];
		} else {
		    print "could not find altfcnspec for $gpio_mode\n";
		    $dirs[$nr] = 0;
		}
	    }
	}
    }
    print HEADER "\n\n";
    for $nr (sort(keys %mode_line)) {
	print HEADER $mode_line{$nr};
    }
    ## now for the bootldr init data
    $gafr_val[0] = 0;
    $gafr_val[1] = 0;
    $gafr_val[2] = 0;
    $gafr_val[3] = 0;
    $gafr_val[4] = 0;
    $gafr_val[5] = 0;

    $gpdr_val[0] = 0;
    $gpdr_val[1] = 0;
    $gpdr_val[2] = 0;

    $gpsr_val[0] = 0;
    $gpsr_val[1] = 0;
    $gpsr_val[2] = 0;
    for ($nr = 0; $nr < 81; $nr++) {
	my $gafr_num = POSIX::floor($nr / 16);
	my $gpdr_num = POSIX::floor($nr / 32);
	my $gafr_offset = $nr % 16;
	my $gpdr_offset = $nr % 32;

	if ($altfcn[$nr]) {
	    $gafr_val[$gafr_num] |= ($altfcn[$nr] << $gafr_offset*2);
	}
	my $dir = $dirs[$nr];
	$gpdr_val[$gpdr_num] |= ($dir << $gpdr_offset) if ($dir);

	my $set = 0;
	$set = $sets[$nr] if ($sets[$nr]);
	print "gpio $nr dir=$dir set=$set\n";
	$gpsr_val[$gpdr_num] |= ($set << $gpdr_offset);

    }
    for ($nr = 0; $nr < 3; $nr++) {
	my $hexstr = sprintf("0x%08x", $gpsr_val[$nr]);
	print HEADER "GPSR", $nr, "_", POSIX::toupper($model), "_InitValue:\t", ".long  ", $hexstr, "\n";
    }
    for ($nr = 0; $nr < 6; $nr++) {
	my $hexstr = sprintf("0x%08x", $gafr_val[$nr]);
	print HEADER $$gafr_name[$nr], "_", POSIX::toupper($model), "_InitValue:\t", ".long  ", $hexstr, "\n";
    }
    for ($nr = 0; $nr < 3; $nr++) {
	my $hexstr = sprintf("0x%08x", $gpdr_val[$nr]);
	print HEADER "GPDR", $nr, "_", POSIX::toupper($model), "_InitValue:\t", ".long  ", $hexstr, "\n";
    }
}

open(HEADER, ">" . $model . "-pxa-gpio.h");
&gen_header();
close(HEADER);
