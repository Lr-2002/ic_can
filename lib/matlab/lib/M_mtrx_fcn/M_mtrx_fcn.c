/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: M_mtrx_fcn.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 2025-10-15 16:51:37
 */

/* Include Files */
#include "M_mtrx_fcn.h"
#include <math.h>
#include <string.h>

/* Function Declarations */
static void ft_1(const double ct[318], double M_mtrx_sym[36]);

/* Function Definitions */
/*
 * Arguments    : const double ct[318]
 *                double M_mtrx_sym[36]
 * Return Type  : void
 */
static void ft_1(const double ct[318], double M_mtrx_sym[36])
{
  double b_ct_idx_132_tmp;
  double b_ct_idx_186_tmp_tmp;
  double b_ct_idx_93_tmp;
  double b_ct_idx_99_tmp;
  double b_t1192_tmp_tmp;
  double b_t1220_tmp;
  double b_t765_tmp;
  double b_t896_tmp;
  double b_t926_tmp_tmp;
  double c_ct_idx_132_tmp;
  double c_t1220_tmp;
  double ct_idx_100_tmp;
  double ct_idx_102;
  double ct_idx_103;
  double ct_idx_112;
  double ct_idx_129;
  double ct_idx_129_tmp;
  double ct_idx_130;
  double ct_idx_130_tmp;
  double ct_idx_130_tmp_tmp;
  double ct_idx_131_tmp;
  double ct_idx_132;
  double ct_idx_132_tmp;
  double ct_idx_136_tmp;
  double ct_idx_137;
  double ct_idx_140;
  double ct_idx_141_tmp;
  double ct_idx_151_tmp;
  double ct_idx_154_tmp;
  double ct_idx_157_tmp;
  double ct_idx_158_tmp;
  double ct_idx_159_tmp;
  double ct_idx_160_tmp;
  double ct_idx_176;
  double ct_idx_177;
  double ct_idx_186_tmp;
  double ct_idx_186_tmp_tmp;
  double ct_idx_348_tmp;
  double ct_idx_349_tmp;
  double ct_idx_353;
  double ct_idx_433;
  double ct_idx_439;
  double ct_idx_439_tmp;
  double ct_idx_446_tmp;
  double ct_idx_454_tmp;
  double ct_idx_458_tmp;
  double ct_idx_484;
  double ct_idx_532_tmp;
  double ct_idx_533_tmp;
  double ct_idx_61_tmp;
  double ct_idx_61_tmp_tmp;
  double ct_idx_62_tmp;
  double ct_idx_63_tmp;
  double ct_idx_73_tmp;
  double ct_idx_81;
  double ct_idx_90;
  double ct_idx_93_tmp;
  double ct_idx_95_tmp;
  double ct_idx_99_tmp;
  double ct_idx_99_tmp_tmp;
  double t1004;
  double t1011;
  double t1016;
  double t1017;
  double t1035;
  double t1041;
  double t1059;
  double t1082;
  double t1084;
  double t1084_tmp;
  double t1084_tmp_tmp;
  double t1085;
  double t1085_tmp;
  double t1112;
  double t1115;
  double t1120;
  double t1122;
  double t1124;
  double t1126;
  double t1152;
  double t1152_tmp;
  double t1152_tmp_tmp;
  double t1153;
  double t1174;
  double t1174_tmp;
  double t1192;
  double t1192_tmp;
  double t1192_tmp_tmp;
  double t1200;
  double t1220;
  double t1220_tmp;
  double t1276;
  double t1282;
  double t1303;
  double t1318;
  double t1327_tmp;
  double t1371;
  double t1447;
  double t1449;
  double t1451;
  double t1451_tmp;
  double t1452;
  double t1452_tmp;
  double t1452_tmp_tmp;
  double t1525;
  double t1529;
  double t1563;
  double t1564;
  double t1568;
  double t1569;
  double t1572;
  double t1573;
  double t1578;
  double t1579;
  double t1631_tmp;
  double t1690;
  double t342;
  double t355;
  double t355_tmp;
  double t425;
  double t435;
  double t438;
  double t439;
  double t460;
  double t467;
  double t468;
  double t468_tmp;
  double t471;
  double t471_tmp;
  double t491;
  double t507;
  double t510;
  double t515;
  double t515_tmp;
  double t518;
  double t518_tmp;
  double t519;
  double t519_tmp;
  double t527;
  double t534;
  double t534_tmp;
  double t584;
  double t584_tmp;
  double t585;
  double t585_tmp;
  double t602;
  double t637;
  double t637_tmp;
  double t645;
  double t648;
  double t650;
  double t679;
  double t684;
  double t690;
  double t703;
  double t704;
  double t705;
  double t706;
  double t711;
  double t714;
  double t718;
  double t722;
  double t742;
  double t743;
  double t744;
  double t745;
  double t747;
  double t755;
  double t760;
  double t761;
  double t761_tmp_tmp;
  double t762;
  double t765;
  double t765_tmp;
  double t783;
  double t785;
  double t785_tmp;
  double t785_tmp_tmp;
  double t810;
  double t813;
  double t814;
  double t815;
  double t818;
  double t824;
  double t829;
  double t831;
  double t839;
  double t841;
  double t850;
  double t855;
  double t855_tmp;
  double t863;
  double t863_tmp;
  double t863_tmp_tmp;
  double t867;
  double t868;
  double t869;
  double t870;
  double t874;
  double t879;
  double t880;
  double t892;
  double t892_tmp;
  double t895;
  double t896;
  double t896_tmp;
  double t913;
  double t914;
  double t915;
  double t926;
  double t926_tmp;
  double t926_tmp_tmp;
  double t927;
  t342 = (ct[52] + ct[81]) + ct[111];
  t425 = ct[129] + ct[176];
  t460 = (ct[104] + ct[139]) + ct[142];
  t468_tmp = ct[32] * ct[248];
  t468 = t468_tmp / 2.0;
  t491 = ct[135] + ct[217];
  t515_tmp = ct[136] - ct[220];
  t515 = ct[20] * t515_tmp * -0.5;
  t518_tmp = ct[93] * ct[260];
  t518 = t518_tmp * ct[257];
  t519_tmp = ct[49] * ct[93];
  t519 = t519_tmp * ct[257];
  t534_tmp = ct[87] * ct[93];
  t534 = t534_tmp * ct[257];
  t584_tmp = ct[59] * ct[222];
  t584 = t584_tmp * ct[267];
  t585_tmp = ct[59] * ct[315];
  t585 = t585_tmp * ct[267];
  t602 = (ct[131] + ct[150]) + ct[209];
  t637_tmp = ct[57] * ct[58];
  t637 = t637_tmp * ct[267];
  t1371 = ct[58] * ct[59];
  t645 = t1371 * ct[269];
  t648 = ct[174] * ct[269];
  t650 = ct[179] * ct[269];
  t679 = (ct[171] + ct[201]) + ct[236];
  t684 = t518_tmp * ct[303];
  t703 = (ct[223] + ct[231]) + ct[238];
  t706 = (ct[204] + ct[226]) + ct[265];
  t839 = (((((ct[113] + ct[116]) + ct[118]) + ct[148]) + ct[155]) + ct[163]) +
         ct[303];
  t863_tmp_tmp = ct[130] - ct[197];
  t863_tmp = ct[173] * t863_tmp_tmp;
  t1318 = ((ct[193] - ct[276]) + ct[282]) + t863_tmp * 0.08395;
  t863 = t637_tmp * t1318;
  t895 = (ct[277] + ct[285]) + ct[297];
  t355_tmp = ct[40] * t342;
  t355 = t355_tmp / 2.0;
  t435 = ct[87] * t425;
  t438 = t518_tmp * t425;
  t439 = t519_tmp * t425;
  t467 = (ct[110] - ct[138]) + ct[147];
  t507 = t534_tmp * t460;
  t510 = ct[22] * t491 / 2.0;
  t527 = ct[54] * t460;
  t704 = (ct[223] / 2.0 - ct[232]) + ct[238] / 2.0;
  t705 = (ct[206] - ct[225]) + ct[264];
  t714 = (ct[212] + ct[226] / 2.0) + ct[265] / 2.0;
  t718 = ct[276] - ct[282];
  t810 = (((ct[146] + ct[221]) - ct[223]) + ct[230]) - ct[238];
  t818 = (ct[152] - ct[224]) + t706;
  t841 = t534_tmp * t839;
  t850 = ct[54] * t839;
  t855_tmp = ct[143] - ct[214];
  t855 = t839 * t855_tmp;
  t867 = (((((ct[115] + ct[154]) - ct[156]) + ct[164]) + ct[167]) + ct[181]) +
         ct[302];
  t868 = (ct[287] + t637) + t645;
  t915 = (ct[280] + ct[285] / 2.0) + ct[300];
  t926_tmp_tmp = ct[186] + t863_tmp;
  b_t926_tmp_tmp = ct[58] * ct[222];
  t839 = b_t926_tmp_tmp * t926_tmp_tmp;
  t926_tmp = (-t585 + t648) + t839;
  t926 = t534_tmp * t926_tmp;
  t1004 = (ct[192] + ct[219]) + t895;
  t1035 = (((ct[191] - ct[195]) + ct[208]) + ct[216]) +
          ((ct[278] + ct[286]) + ct[298]);
  t471_tmp = ct[30] * t467;
  t471 = t471_tmp / 2.0;
  t711 = (ct[213] - ct[225] / 2.0) + ct[264] / 2.0;
  t722 = ct[279] - ct[282] / 2.0;
  t742 = (ct[254] + ct[258]) + -t460 * t855_tmp;
  t743 = (ct[255] + ct[263]) + t527;
  t744 = (ct[254] * 2.0 + ct[259]) + t460 * t855_tmp * -2.0;
  t755 = (t439 + t518) + ct[271];
  t765_tmp = ct[132] - ct[218];
  t1303 = ct[63] * t765_tmp;
  b_t765_tmp = (-t438 + t519) + t1303;
  t765 = -ct[93] * b_t765_tmp;
  t785_tmp_tmp = ct[107] * t765_tmp;
  t785_tmp = (t435 - t534) + t785_tmp_tmp;
  t785 = ct[73] * t785_tmp;
  t813 = ct[87] * t810;
  t814 = t518_tmp * t810;
  t815 = t519_tmp * t810;
  t824 = ct[107] * t818;
  t829 = ct[63] * t818;
  t831 = ct[71] * t818;
  t869 = ct[93] * t867;
  t870 = ct[125] * t867;
  t874 = ct[87] * t868;
  t879 = t518_tmp * t868;
  t880 = t519_tmp * t868;
  t810 = (((ct[141] + ct[153]) + ct[183]) + ct[227]) + t705;
  t892_tmp = (-(t585 * 2.0) + t648 * 2.0) + t839 * 2.0;
  t892 = ct[22] * t892_tmp * -0.5;
  t896_tmp = ct[58] * ct[315];
  t818 = t896_tmp * t926_tmp_tmp;
  b_t896_tmp = (t584 * 2.0 - t650 * 2.0) + t818 * 2.0;
  t896 = ct[20] * b_t896_tmp * -0.5;
  t1011 = t1371 * t1004;
  t1016 = ct[174] * t1004;
  t1017 = ct[179] * t1004;
  t1004 = ct[60] * t1035;
  t1041 = b_t926_tmp_tmp * t1035;
  t1035 *= t896_tmp;
  t690 = ct[253] - t507;
  t745 = (ct[255] * 2.0 - ct[262]) + t527 * 2.0;
  t747 = ct[42] * t744 / 2.0;
  t760 = (t439 * 2.0 + t518 * 2.0) + ct[271] * 2.0;
  t761_tmp_tmp = (-(t438 * 2.0) + t519 * 2.0) + t1303 * 2.0;
  t839 = ct[32] * t761_tmp_tmp;
  t761 = t839 * -0.5;
  t762 = t839 / 2.0;
  t783 = t534_tmp * t755;
  t527 = ct[54] * t755;
  t913 = t518_tmp * t810;
  t914 = t519_tmp * t810;
  t927 = t534_tmp * t810;
  t1059 = t841 + t869;
  t839 = t755 * t855_tmp;
  t810 = ct[125] * b_t765_tmp;
  t1371 = ct[67] * t785_tmp;
  t1084_tmp_tmp = (t839 * 2.0 + t810 * 2.0) + t1371 * 2.0;
  t1084_tmp = ct[42] * t1084_tmp_tmp;
  t1084 = t1084_tmp / 2.0;
  t1152_tmp_tmp = ct[87] * (ct[85] - ct[86]);
  t1152_tmp =
      (((((((ct[112] + ct[142] * 0.045) - ct[145]) + ct[151]) - ct[172]) -
         ct[175]) +
        ct[239]) +
       t1152_tmp_tmp) +
      t637_tmp * ct[103];
  t1152 = (t855 + t870) + -ct[67] * t1152_tmp;
  t1153 = (t850 + ct[237] * t867) + -ct[73] * t1152_tmp;
  t1174_tmp = (t584 - t650) + t818;
  t1174 = (t874 + -ct[107] * t1174_tmp) + t926;
  t1192_tmp_tmp = ct[63] * t1174_tmp;
  b_t1192_tmp_tmp = t519_tmp * t926_tmp;
  t1192_tmp = (t879 + t1192_tmp_tmp) + b_t1192_tmp_tmp;
  t1192 = -ct[93] * t1192_tmp;
  t1200 = ct[143] * t1192_tmp;
  t1220_tmp = ct[71] * t1174_tmp;
  b_t1220_tmp = t518_tmp * t926_tmp;
  c_t1220_tmp = (-t880 + t1220_tmp) + b_t1220_tmp;
  t1220 = t855_tmp * c_t1220_tmp;
  ct_idx_61_tmp_tmp = ct[93] * b_t765_tmp;
  ct_idx_61_tmp = t783 + ct_idx_61_tmp_tmp;
  ct_idx_62_tmp = -ct[43] * ct[82];
  ct_idx_63_tmp = ct[40] * ct[78];
  ct_idx_81 = t841 / 2.0 + t869 / 2.0;
  ct_idx_90 = (-ct[143] * b_t765_tmp + t785) + t527;
  ct_idx_93_tmp = (t839 + t810) + t1371;
  b_ct_idx_93_tmp = ct[41] * ct[67];
  ct_idx_95_tmp = ct[39] * ct[78];
  ct_idx_99_tmp_tmp = t519_tmp * t855_tmp;
  ct_idx_99_tmp = (-ct[51] + ct[101]) + ct_idx_99_tmp_tmp;
  b_ct_idx_99_tmp = ct[41] * ct_idx_99_tmp;
  ct_idx_100_tmp = ct[39] * t342;
  ct_idx_102 = (-t813 + t824) + t927;
  ct_idx_103 = (t815 - t831) + t913;
  ct_idx_112 = (t855 / 2.0 + t870 / 2.0) + ct[67] * t1152_tmp * -0.5;
  ct_idx_129_tmp = (-(t880 * 2.0) + t1220_tmp * 2.0) + b_t1220_tmp * 2.0;
  ct_idx_129 = ct[30] * ct_idx_129_tmp * -0.5;
  ct_idx_130_tmp_tmp =
      (t879 * 2.0 + t1192_tmp_tmp * 2.0) + b_t1192_tmp_tmp * 2.0;
  ct_idx_130_tmp = ct[32] * ct_idx_130_tmp_tmp;
  ct_idx_130 = ct_idx_130_tmp / 2.0;
  ct_idx_131_tmp = ct[31] * ct[260];
  ct_idx_132_tmp = ct[29] * ct[49];
  ct_idx_136_tmp = ct[28] * ct[49] * ct[93];
  ct_idx_137 = ct[67] * t1174;
  ct_idx_140 = ct[73] * t1174;
  ct_idx_141_tmp = ct[29] * ct[260] * ct[93];
  ct_idx_151_tmp = ct[33] * ct[235];
  ct_idx_154_tmp = ct[30] * ct[235];
  ct_idx_157_tmp = -ct[31] * ct[246];
  ct_idx_158_tmp = ct[28] * t460;
  ct_idx_159_tmp = ct[29] * t467;
  ct_idx_160_tmp = ct[29] * ct[246];
  ct_idx_176 = (t863 + t1011) + t1004;
  ct_idx_177 = (-ct[222] * ct[59] * t1318 + t1017) + t1035;
  ct_idx_186_tmp_tmp = t534_tmp * c_t1220_tmp;
  b_ct_idx_186_tmp_tmp = ct[93] * t1192_tmp;
  ct_idx_186_tmp = b_ct_idx_186_tmp_tmp + ct_idx_186_tmp_tmp;
  ct_idx_348_tmp = ct[188] - ct[203];
  ct_idx_349_tmp = ct[187] + t863_tmp * 2.0;
  ct_idx_353 = ((ct[84] - ct[90]) - ct[165]) + ct[168];
  ct_idx_433 = ct[40] * t745 / 2.0;
  ct_idx_439_tmp = ct[30] * t760;
  ct_idx_439 = ct_idx_439_tmp / 2.0;
  ct_idx_446_tmp = ct[30] * ct[87];
  ct_idx_454_tmp = ct[38] * ct[73];
  ct_idx_458_tmp = ct[39] * ct[67];
  ct_idx_484 = t585_tmp * t1318;
  ct_idx_532_tmp = ct[21] * ct[315] * ct[59];
  ct_idx_533_tmp = ct[19] * ct[315] * ct[59];
  t1276 = (t1011 / 2.0 + t863 / 2.0) + t1004 / 2.0;
  t1282 = (t1017 / 2.0 + t584_tmp * t1318 * -0.5) + t1035 / 2.0;
  t1327_tmp =
      (((((((t439 * 0.045 - ct[97] * t425) + t519 * 0.090162) + t518 * 0.045) +
          ct[271] * 0.045) +
         t813) -
        t824) -
       t927) +
      t1303 * 0.090162;
  t1011 = ct[73] * t1327_tmp;
  t839 = ((t585 * -0.02575 + ct[291]) + t648 * 0.02575) + ct_idx_176;
  t810 = ((t585 * -0.28435 + ct[294]) + t648 * 0.28435) + ct_idx_177;
  t818 = (((((((t584 * -0.28435 + ct[289]) + ct[292]) + ct[137] * ct[267]) +
             t645 * 0.02575) +
            t650 * 0.28435) +
           ct_idx_484) -
          t1016) +
         t1041;
  t1082 = (ct[143] * b_t765_tmp * -2.0 + t785 * 2.0) + t527 * 2.0;
  t1112 = (-(t813 / 2.0) + t824 / 2.0) + t927 / 2.0;
  t1115 = (t814 + t829) - t914;
  t1122 = (t815 / 2.0 - t831 / 2.0) + t913 / 2.0;
  t1126 = t518_tmp * ct_idx_103;
  t927 = (((((t435 * -0.045 - t438 * 0.06565) + t519 * 0.06565) + ct[266]) +
           ct[53] * ct[257]) +
          ct[274]) +
         ct_idx_103;
  t869 = ct[87] * t839;
  t855 = t518_tmp * t839;
  t1004 = t519_tmp * t839;
  t1035 = ct[107] * t810;
  t841 = ct[63] * t810;
  t810 *= ct[71];
  t1371 = t518_tmp * t818;
  t527 = t519_tmp * t818;
  t818 *= t534_tmp;
  t1447 = (t1200 + ct_idx_140) + -ct[54] * c_t1220_tmp;
  t1085_tmp = ct[40] * t1082;
  t1085 = t1085_tmp / 2.0;
  t1120 = (t814 / 2.0 + t829 / 2.0) - t914 / 2.0;
  t1124 = t519_tmp * t1115;
  t1303 = t534_tmp * t927;
  t870 = ct[54] * t927;
  t839 = (((((t439 * 0.06565 + ct[91] * t425) + t518 * 0.06565) + ct[275]) +
           ct[271] * 0.06565) +
          t534 * -0.090162) +
         t1115;
  t1449 = (t1200 * 2.0 + ct_idx_140 * 2.0) + ct[54] * c_t1220_tmp * -2.0;
  t863_tmp = ct[125] * t1192_tmp;
  t1452_tmp_tmp = (-(ct_idx_137 * 2.0) + t1220 * 2.0) + t863_tmp * 2.0;
  t1452_tmp = ct[42] * t1452_tmp_tmp;
  t1452 = t1452_tmp / 2.0;
  t1569 = (t1004 + t810) + t1371;
  t1318 = ct[93] * t839;
  t863 = ct[125] * t839;
  t839 *= ct[143];
  t1451_tmp = ct[40] * t1449;
  t1451 = t1451_tmp / 2.0;
  t1563 = (t869 + t1035) - t818;
  t1568 = (-t855 + t841) + t527;
  t1573 = (t1004 / 2.0 + t810 / 2.0) + t1371 / 2.0;
  t1578 = t518_tmp * t1569;
  t1631_tmp = ct[107] * t1174_tmp;
  t1017 = (((((ct[261] * t868 + t879 * 0.06565) + t1631_tmp * -0.045) +
             b_t1192_tmp_tmp * 0.06565) +
            ct[53] * t926_tmp) +
           t1192_tmp_tmp * 0.06565) +
          t1569;
  t1525 = (t870 + t839) + t1011;
  t1564 = (t869 / 2.0 + t1035 / 2.0) - t818 / 2.0;
  t1572 = (-(t855 / 2.0) + t841 / 2.0) + t527 / 2.0;
  t1579 = t519_tmp * t1568;
  t810 = t534_tmp * t1017;
  t869 = ct[54] * t1017;
  t841 = (((((t880 * 0.06565 + b_t1220_tmp * -0.06565) + ct[91] * t868) +
            t1631_tmp * -0.090162) +
           t1220_tmp * -0.06565) +
          ct[91] * ct[93] * t926_tmp) +
         t1568;
  t1529 = (t870 / 2.0 + t839 / 2.0) + t1011 / 2.0;
  t839 = ct[93] * t841;
  t527 = ct[125] * t841;
  t1690 = t810 + t839;
  t879 = t810 / 2.0 + t839 / 2.0;
  t519 = (-ct_idx_137 + t1220) + t863_tmp;
  ct_idx_73_tmp = ct[40] * ct[82];
  t518 = ct[38] * ct[184];
  t439 = ct[39] * t1082;
  t438 = (-t1016 + ct_idx_484) + t1041;
  b_ct_idx_132_tmp = ct[76] * t855_tmp;
  c_ct_idx_132_tmp = (ct[144] - ct[198]) + b_ct_idx_132_tmp;
  b_ct_idx_132_tmp = (ct[149] - ct[202]) + b_ct_idx_132_tmp / 2.0;
  ct_idx_132 =
      (((((((((((((ct[98] + ct[242]) + ct[247]) + ct[312]) + ct[314]) +
               ct[157]) +
              ct[158]) +
             ct_idx_454_tmp * ct[184]) +
            ct[228]) -
           ct_idx_458_tmp * t342 / 2.0) +
          ct[229]) -
         ct[45] * ((ct[196] + ct[234]) + ct[133] * ct[184])) -
        ct[44] * ((ct[194] + ct[233]) + ct[240])) -
       ct[46] *
           (((ct[161] * ct[184] + ct[251]) + ct[67] * ct[283]) + ct[295])) +
      ct[47] * (((((ct[252] + ct[256]) - ct[160] * ct[283]) - ct[159] * t602) +
                 ct[162] * c_ct_idx_132_tmp) +
                ct[161] * b_ct_idx_132_tmp);
  ct_idx_140 = -ct[43] * t690;
  t914 = ct[39] * t1449;
  t1200 = -ct[41] * t742;
  t829 = ct[38] * t743;
  t1220 = ct[39] * t742;
  t814 = ct[39] * t745;
  ct_idx_137 = t1303 - t1318;
  t1035 = ct[67] * t1327_tmp;
  t1004 = t927 * t855_tmp;
  b_t1192_tmp_tmp = (-t863 + t1035) + t1004;
  t815 = (-(t850 / 2.0) + ct[143] * t867 / 2.0) + ct[73] * t1152_tmp / 2.0;
  t650 = (ct_idx_484 / 2.0 - t1016 / 2.0) + t1041 / 2.0;
  t824 = (-ct[175] + ct[239]) + t1152_tmp_tmp;
  t813 = ct[42] * ct[82];
  t831 = ct[43] * ct[82];
  t785 =
      (((((((((((((((((((((((((((((ct[69] + ct[79]) + ct[100]) + ct[105]) +
                                ct[126]) +
                               ct[243]) +
                              ct[244]) +
                             ct[245]) +
                            ct_idx_131_tmp * ct[253]) +
                           ct_idx_132_tmp * ct[253]) +
                          ct_idx_136_tmp * t460) +
                         ct[32] * ct[87] * ct[248] * -0.5) +
                        ct_idx_446_tmp * t467 * -0.5) +
                       ct_idx_141_tmp * t467 / 2.0) +
                      t831 * t690) +
                     t355_tmp * t690 * -0.5) +
                    ct[190] * t690) +
                   t813 * t744 * -0.5) +
                  ct_idx_73_tmp * t745 * -0.5) +
                 t1200 * ct_idx_99_tmp) +
                t518 * t743) +
               ct_idx_100_tmp * t742 / 2.0) +
              t814 * ct_idx_99_tmp * -0.5) +
             ct[34] * (((ct[249] - ct[307]) + ct[87] * ct[253] * 0.02575) +
                       t518_tmp * t824)) -
            ct[35] *
                (((ct[250] + ct[55] * t460) + ct[301]) + ct[87] * ct[303])) -
           ct[36] *
               (((ct[253] * ct[260] * 0.02575 + t519_tmp * t460 * 0.02575) +
                 t684) +
                ct[308])) +
          ct[37] *
              (((((ct[306] + ct[309]) + t684 * 0.012875) + ct[308] * 0.012875) +
                t519_tmp * ct[305] * 0.02575) +
               t518_tmp * t679 * 0.02575)) +
         -ct[44] *
             (((-(ct[207] * t742) + ct[82] * t1152) + t1059 * ct_idx_99_tmp) +
              t690 * c_ct_idx_132_tmp)) -
        ct[45] * (((-(ct[283] * t690) + ct[207] * t743) + ct[184] * t1059) +
                  ct[82] * t1153)) +
       ct[46] *
           (((ct[283] * t742 + -t743 * c_ct_idx_132_tmp) - ct[184] * t1152) +
            t1153 * ct_idx_99_tmp)) +
      ct[47] * (((((ct[215] * t1059 + ct_idx_81 * ct[207]) - t602 * t1153) +
                  ct[283] * t815) +
                 ct_idx_112 * c_ct_idx_132_tmp) +
                t1152 * b_ct_idx_132_tmp);
  t839 =
      (((((t880 * -0.045 + ct[97] * t868) + t1563) + t1192_tmp_tmp * 0.090162) +
        t1220_tmp * 0.045) +
       b_t1220_tmp * 0.045) +
      ct[49] * ct[92] * t926_tmp;
  t818 = ct[67] * t839;
  t810 = t1017 * t855_tmp;
  t584 = (t527 + t818) + t810;
  t839 *= ct[73];
  t1371 = (t869 + ct[237] * t841) + t839;
  t648 = (-(t863 / 2.0) + t1035 / 2.0) + t1004 / 2.0;
  t1318 = t1303 / 2.0 - t1318 / 2.0;
  t1303 = (-ct[180] + ct[241]) + t1152_tmp_tmp / 2.0;
  t913 = ct[40] * t690;
  t585 = ct[19] * ct[222];
  t863 =
      (((((((((((((((((((((((((((((((((((((((((ct[88] + ct[89]) + ct[123]) +
                                             ct[124]) +
                                            ct[69] * t425) +
                                           ct[79] * t425) +
                                          ct[126] * t425) +
                                         ct_idx_532_tmp * ct[257]) -
                                        t585 * ct[59] * ct[257]) +
                                       ct[268]) +
                                      ct_idx_533_tmp * t515_tmp / 2.0) +
                                     ct[22] * ct[57] * ct[58] * t491 * -0.5) +
                                    t637_tmp * t515) +
                                   -ct[33] * ct[235] * t785_tmp) +
                                  t468_tmp * t785_tmp * -0.5) +
                                 ct[235] * t761) +
                                t471_tmp * t785_tmp * -0.5) +
                               ct[235] * ct_idx_439) +
                              ct_idx_157_tmp * b_t765_tmp) +
                             ct_idx_158_tmp * t755) +
                            ct_idx_160_tmp * t760 / 2.0) +
                           ct_idx_159_tmp * b_t765_tmp * -0.5) -
                          ct[24] * (((b_t926_tmp_tmp * t425 * -0.08395 +
                                      ct[86] * ct[257]) +
                                     t585_tmp * t703) +
                                    t637_tmp * t705)) -
                         ct[25] *
                             (((ct[95] * t425 + ct[272]) + t584_tmp * t703) +
                              t637_tmp * t706)) -
                        ct[26] *
                            (((ct[95] * ct[257] + ct[273]) + t585_tmp * t706) -
                             t584_tmp * t705)) +
                       ct_idx_140 * ct_idx_61_tmp) +
                      t747 * ct_idx_61_tmp) +
                     ct_idx_433 * ct_idx_61_tmp) +
                    t690 * t1084) +
                   t913 * t1082 * -0.5) +
                  t1200 * ct_idx_93_tmp) +
                 t829 * ct_idx_90) +
                t1220 * t1082 / 2.0) +
               t814 * ct_idx_93_tmp * -0.5) +
              ct[27] * (((((ct[60] * t703 * 0.041975 + ct[86] * t704) +
                           t896_tmp * t706 * 0.041975) -
                          b_t926_tmp_tmp * t705 * 0.041975) +
                         b_t926_tmp_tmp * t711 * -0.08395) +
                        ct[95] * t714)) +
             -ct[34] * (((t824 * b_t765_tmp + ct_idx_102 * ct[246]) -
                         ct[235] * t1115) +
                        ct[302] * t785_tmp)) +
            -ct[35] *
                (((-(ct_idx_102 * t460) + ct_idx_103 * ct[235]) + t755 * t824) +
                 ct[303] * t785_tmp)) -
           ct[36] * (((-ct[303] * b_t765_tmp + ct[302] * t755) + t460 * t1115) -
                     ct_idx_103 * ct[246])) +
          -ct[37] *
              (((((-(ct[302] * t1120) - ct[305] * t1115) + ct[303] * t1122) +
                 ct_idx_103 * t679) +
                t1112 * t824) +
               ct_idx_102 * t1303)) +
         ct[44] * (((-t1059 * ct_idx_93_tmp + t1152 * ct_idx_61_tmp) +
                    -t742 * ct_idx_137) +
                   t690 * b_t1192_tmp_tmp)) +
        ct[45] * (((-(t1059 * ct_idx_90) + t690 * t1525) + t743 * ct_idx_137) +
                  t1153 * ct_idx_61_tmp)) +
       ct[46] *
           (((-(ct_idx_90 * t1152) + t742 * t1525) + t1153 * ct_idx_93_tmp) +
            t743 * b_t1192_tmp_tmp)) +
      -ct[47] * (((((t1153 * t1529 + -t1525 * t815) + ct_idx_81 * ct_idx_137) +
                   t1059 * t1318) +
                  ct_idx_112 * b_t1192_tmp_tmp) +
                 t1152 * t648);
  t1011 = (t527 / 2.0 + t818 / 2.0) + t810 / 2.0;
  t1035 = (t869 / 2.0 - ct[143] * t841 / 2.0) + t839 / 2.0;
  t810 = ct[270] * t863_tmp_tmp;
  t818 = (-ct[278] + ct[296]) + t810;
  t810 = (-ct[281] + ct[299]) + t810 / 2.0;
  t1004 = ct[20] * t868;
  t527 = ct[29] * t760;
  t869 = ct[9] * ct[65];
  t841 = ct[19] * ct[257];
  t1017 = ct[23] * t425;
  t927 = ct[43] * ct_idx_186_tmp;
    t839 = (((((((((((((((((((((((((((((((((((((((((((((((((((ct[177] + ct[182]) + ct[210]) + ct[211]) + ct[83] * ct[267]) + ct[10] * ct[77] * ct[267] * -0.5) + ct[8] * ct[72] * ct[269]) + ct[288]) + t869 * ct_idx_348_tmp / 2.0) + ct[290]) - t1017 * t868) + t425 * t892) + t425 * t896) + t510 * t868) + t1004 * t515_tmp / 2.0) + ct[21] * ct[257] * t926_tmp) + ct[18] * t765_tmp * t1174_tmp) + t841 * b_t896_tmp / 2.0) + ct[19] * t515_tmp * t926_tmp / 2.0) + ct[15] * ((ct[96] * ct[269] + ct[119] * ct[267]) - ct[72] * t718)) - ct[14] * ((ct[293] + ct[120] * ct[267]) + ct[65] * t718)) + -ct[16] * (((ct[120] * ct[269] - ct[65] * t895) + ct[119] * t926_tmp_tmp) + ct[72] * t818)) + -ct[33] * t1174 * t785_tmp) + ct_idx_130 * t785_tmp) + t761 * t1174) + ct_idx_129 * t785_tmp) + t1174 * ct_idx_439) + ct[31] * b_t765_tmp * t1192_tmp) + ct[28] * t755 * c_t1220_tmp) + t527 * t1192_tmp * -0.5) - ct[29] * b_t765_tmp * ct_idx_129_tmp / 2.0) - ct[17] * (((((ct[99] * t718 + ct[96] * t722) + ct[121] * t895) + ct[317]) + ct[119] * t915) + -ct[120] * t810)) + t927 * ct_idx_61_tmp) + t1084_tmp * -0.5 * ct_idx_186_tmp) + t1085 * ct_idx_186_tmp) + t1451_tmp * ct_idx_61_tmp * -0.5) + t1452_tmp * -0.5 * ct_idx_61_tmp) + ct[24] * (((t705 * t868 + -t703 * t926_tmp) + -t425 * t438) + ct_idx_176 * ct[257])) + ct[25] * (((t706 * t868 + t703 * t1174_tmp) + ct_idx_177 * t425) + -ct_idx_176 * t765_tmp)) + ct[41] * ct_idx_93_tmp * t519) - ct[38] * ct_idx_90 * t1447) + t914 * ct_idx_93_tmp / 2.0) + t439 * t519 * -0.5) + -ct[26] * (((-(ct_idx_177 * ct[257]) + t765_tmp * t438) + t706 * t926_tmp) + t705 * t1174_tmp)) - ct[27] * (((((t703 * t1276 + ct_idx_176 * t704) + -t711 * t438) + -t705 * t650) + ct_idx_177 * t714) + t706 * t1282)) + ct[34] * (((t1115 * t1174 + -t1563 * b_t765_tmp) + ct_idx_102 * t1192_tmp) + t1568 * t785_tmp)) - ct[35] * (((-ct_idx_102 * c_t1220_tmp + ct_idx_103 * t1174) + -t1569 * t785_tmp) + t755 * t1563)) + -ct[36] * (((-(t755 * t1568) + ct_idx_103 * t1192_tmp) + t1115 * c_t1220_tmp) + t1569 * b_t765_tmp)) - ct[37] * (((((ct_idx_102 * t1564 + t1112 * t1563) + t1115 * t1572) + t1120 * t1568) - t1122 * t1569) - ct_idx_103 * t1573)) + ct[44] * (((ct_idx_137 * t519 - ct_idx_186_tmp * b_t1192_tmp_tmp) - ct_idx_61_tmp * t584) + t1690 * ct_idx_93_tmp)) + -ct[45] * (((-(ct_idx_90 * t1690) + ct_idx_61_tmp * t1371) + t1525 * ct_idx_186_tmp) + t1447 * ct_idx_137)) + -ct[46] * (((t1371 * ct_idx_93_tmp + -ct_idx_90 * t584) + t1525 * t519) + t1447 * b_t1192_tmp_tmp)) + ct[47] * (((((t1011 * b_t1192_tmp_tmp + t584 * t648) + t1529 * t1371) + t879 * ct_idx_137) + t1690 * t1318) + t1525 * t1035);
    t870 = t1371 * 2.0;
    t855 = ct[57] * ct[106];
    t863_tmp = ct[57] * ct[310];
    M_mtrx_sym[0] =
        (((((((((((((((ct[48] - ct[36] * (t1569 * t1192_tmp * 2.0 -
                                          t1568 * c_t1220_tmp * 2.0)) -
                      ct[45] * (t1447 * t1690 * 2.0 + ct_idx_186_tmp * t870)) +
                     ct[37] * ((t1563 * t1564 * 2.0 + t1568 * t1572 * 2.0) +
                               t1569 * t1573 * 2.0)) -
                    ct[35] *
                        (t1174 * t1569 * 2.0 + t1563 * c_t1220_tmp * 2.0)) +
                   ((((ct[41] * (t519 * t519) -
                       ct[14] *
                           (t718 * t926_tmp_tmp * 2.0 + ct[267] * t818 * 2.0)) +
                      ct[16] *
                          (t895 * t926_tmp_tmp * 2.0 - ct[269] * t818 * 2.0)) +
                     ct[47] * ((t1690 * t879 * 2.0 + t584 * (t1011 * 2.0)) +
                               t1035 * t870)) +
                    ct[27] *
                        ((ct_idx_176 * t1276 * 2.0 + ct_idx_177 * t1282 * 2.0) +
                         t650 * t438 * 2.0))) +
                  ((((ct[17] * ((t718 * t722 * 2.0 + t895 * t915 * 2.0) +
                                t818 * t810 * 2.0) +
                      ct[18] * (t1174_tmp * t1174_tmp)) +
                     ct[21] * (t926_tmp * t926_tmp)) +
                    ct[11] * (t926_tmp_tmp * t926_tmp_tmp)) -
                   ct[25] * (ct_idx_176 * t1174_tmp * 2.0 +
                             ct_idx_177 * t868 * 2.0))) +
                 -ct[46] * (t1371 * (t519 * 2.0) + t1447 * t584 * 2.0)) +
                (ct[7] *
                     ((ct[199] *
                           ((t855 * -0.0825 + ct[62] * ct[106] * 0.01565) +
                            t863_tmp * 0.01565) *
                           2.0 +
                       ct[200] *
                           ((t855 * 0.01565 + t863_tmp * 0.0825) -
                            ct[62] * ct[310] * 0.01565) *
                           2.0) +
                      ct[185] * (ct[59] * 0.0825 + ct[57] * ct[59] * 0.01565) *
                          2.0) +
                 ct[31] * (t1192_tmp * t1192_tmp))) +
               ((((((((ct[26] * (ct_idx_177 * t926_tmp * 2.0 -
                                 t1174_tmp * t438 * 2.0) +
                       ct[0] * (ct[173] * ct[173])) +
                      ct[2] * (ct[178] * ct[178])) +
                     ct[8] * (ct[269] * ct[269])) +
                    ct[13] * (ct[267] * ct[267])) +
                   ct[23] * (t868 * t868)) +
                  ct[33] * (t1174 * t1174)) +
                 ct[38] * (t1447 * t1447)) +
                ct[44] *
                    (t1690 * t519 * 2.0 + ct_idx_186_tmp * (t584 * -2.0)))) +
              (((-ct[34] * (t1563 * t1192_tmp * 2.0 + t1174 * t1568 * 2.0) -
                 ct[6] * (ct[173] * ct[199] * 2.0 - ct[178] * ct[200] * 2.0)) -
                ct[15] * (ct[269] * t718 * 2.0 + ct[267] * t895 * 2.0)) +
               ct[24] * (ct_idx_176 * t926_tmp * 2.0 + t868 * t438 * 2.0))) +
             (ct[43] * (ct_idx_186_tmp * ct_idx_186_tmp) +
              ct[28] * (c_t1220_tmp * c_t1220_tmp))) +
            ((ct[30] * t1174 * ct_idx_129_tmp + t914 * t519) +
             ct[19] * t926_tmp * b_t896_tmp)) +
           (-ct[42] * ct_idx_186_tmp * t1452_tmp_tmp + t1004 * b_t896_tmp)) +
          ((((((ct[22] * t868 * t892_tmp + ct[12] * ct[267] * ct_idx_349_tmp) -
               ct[32] * t1174 * ct_idx_130_tmp_tmp) -
              ct[1] * ct[173] * ct[178] * 2.0) -
             ct[4] * ct[178] * ct[185] * 2.0) -
            ct[5] * ct[173] * ct[185] * 2.0) +
           ct[9] * t926_tmp_tmp * ct_idx_348_tmp)) +
         (-ct[40] * t1449 * ct_idx_186_tmp +
          ct[10] * ct[267] * ct_idx_348_tmp)) +
        -ct[29] * t1192_tmp * ct_idx_129_tmp;
    M_mtrx_sym[1] = t839;
    t870 =
        ((((((((((((((((((((((((((((((((((((((((((ct[10] * ct_idx_348_tmp *
                                                      -0.5 +
                                                  ct[12] * ct_idx_349_tmp *
                                                      -0.5) +
                                                 ct[15] * t895) +
                                                ct[316]) +
                                               ct[70] * t868) -
                                              ct[69] * t868) +
                                             -ct[19] * ct[222] * ct[59] *
                                                 t926_tmp) +
                                            -ct[18] * ct[222] * ct[59] *
                                                t1174_tmp) +
                                           ct_idx_532_tmp * t926_tmp) +
                                          ct_idx_533_tmp * b_t896_tmp / 2.0) -
                                         ct[126] * t868) +
                                        t637_tmp * t892) +
                                       t637_tmp * t896) +
                                      ct_idx_151_tmp * t1174) +
                                     ct_idx_130_tmp * -0.5 * ct[235]) +
                                    t468 * t1174) +
                                   ct_idx_154_tmp * ct_idx_129_tmp / 2.0) +
                                  t471 * t1174) +
                                 ct_idx_157_tmp * t1192_tmp) +
                                ct_idx_158_tmp * c_t1220_tmp) +
                               ct_idx_159_tmp * t1192_tmp * -0.5) +
                              ct_idx_160_tmp * ct_idx_129_tmp / 2.0) +
                             ct_idx_140 * ct_idx_186_tmp) +
                            t747 * ct_idx_186_tmp) +
                           ct_idx_433 * ct_idx_186_tmp) +
                          t690 * t1451) +
                         t690 * t1452) +
                        t1200 * t519) -
                       t829 * t1447) -
                      t1220 * t1449 / 2.0) +
                     t814 * t519 * -0.5) +
                    -ct[24] * (((ct[94] * t868 - t585_tmp * ct_idx_176) +
                                ct[86] * t926_tmp) +
                               t637_tmp * t438)) +
                   ct[26] * (((t896_tmp * t926_tmp * -0.08395 +
                               t585_tmp * ct_idx_177) +
                              ct[94] * t1174_tmp) +
                             t584_tmp * t438)) +
                  ct[25] * (((ct[95] * t868 + ct[86] * t1174_tmp) +
                             t584_tmp * ct_idx_176) +
                            t637_tmp * ct_idx_177)) -
                 ct[27] *
                     (((((ct[60] * ct_idx_176 * 0.041975 + ct[86] * t1276) +
                         t896_tmp * ct_idx_177 * 0.041975) +
                        b_t926_tmp_tmp * t438 * 0.041975) +
                       ct[94] * t650) +
                      ct[95] * t1282)) +
                -ct[34] *
                    (((t824 * t1192_tmp - ct[302] * t1174) - ct[246] * t1563) +
                     ct[235] * t1568)) -
               ct[35] *
                   (((t824 * c_t1220_tmp - ct[303] * t1174) + ct[235] * t1569) +
                    t460 * t1563)) +
              ct[36] * (((-ct[302] * c_t1220_tmp + ct[303] * t1192_tmp) +
                         ct[246] * t1569) +
                        t460 * t1568)) -
             ct[37] * (((((-t1563 * t1303 + -t1564 * t824) + ct[302] * t1572) +
                         ct[305] * t1568) +
                        ct[303] * t1573) +
                       t679 * t1569)) -
            ct[44] *
                (((-t1152 * ct_idx_186_tmp + t1059 * t519) + t742 * t1690) +
                 -t690 * t584)) +
           ct[45] * (((t1153 * ct_idx_186_tmp + t1059 * t1447) + t743 * t1690) +
                     t690 * t1371)) +
          ct[46] *
              (((t1152 * t1447 + t742 * t1371) + t743 * t584) + t1153 * t519)) +
         -ct[47] * (((((ct_idx_81 * t1690 + t1059 * t879) - t1371 * t815) +
                      ct_idx_112 * t584) +
                     t1152 * t1011) +
                    t1153 * t1035)) -
        ct[13] * ct[267];
    M_mtrx_sym[2] = t870;
    t855 =
        ((((((((((((((((((((((((((((t892 + t896) + ct[166] * t1174) +
                                  ct[169] * t1174) -
                                 ct[98] * t1174) +
                                ct_idx_131_tmp * t1192) +
                               ct_idx_132_tmp * t1192) +
                              ct_idx_136_tmp * c_t1220_tmp) +
                             ct_idx_129 * ct[87]) +
                            ct_idx_130 * ct[87]) +
                           ct_idx_141_tmp * ct_idx_129_tmp / 2.0) +
                          -ct[24] * t438) +
                         ct[25] * ct_idx_177) +
                        ct_idx_62_tmp * ct_idx_186_tmp) +
                       t355 * ct_idx_186_tmp) +
                      ct[189] * ct_idx_186_tmp) +
                     ct[82] * t1451) +
                    ct[82] * t1452) +
                   b_ct_idx_99_tmp * t519) -
                  t518 * t1447) +
                 ct_idx_100_tmp * t519 * -0.5) +
                t914 * ct_idx_99_tmp / 2.0) +
               ct[34] * (((t519_tmp * t1174 * 0.02575 +
                           t534_tmp * t1192_tmp * -0.02575) +
                          t518_tmp * t1563) +
                         ct[87] * t1568)) +
              ct[35] * (((t518_tmp * t1174 * 0.02575 +
                          ct_idx_186_tmp_tmp * -0.02575) -
                         t519_tmp * t1563) +
                        ct[87] * t1569)) +
             ct[36] * (((t519_tmp * c_t1220_tmp * -0.02575 +
                         t518_tmp * t1192_tmp * 0.02575) +
                        t1578) +
                       t1579)) -
            ct[37] * (((((-(t534_tmp * t1563 * 0.012875) +
                          t534_tmp * t1564 * -0.02575) +
                         t1578 * 0.012875) +
                        t1579 * 0.012875) +
                       t519_tmp * t1572 * 0.02575) +
                      t518_tmp * t1573 * 0.02575)) +
           ct[45] * (((-ct[283] * ct_idx_186_tmp + ct[207] * t1447) +
                      ct[184] * t1690) +
                     ct[82] * t1371)) +
          ct[44] * (((ct_idx_186_tmp * c_ct_idx_132_tmp + -ct[207] * t519) +
                     t1690 * ct_idx_99_tmp) +
                    ct[82] * t584)) -
         ct[46] *
             (((-t1447 * c_ct_idx_132_tmp + ct[283] * t519) + -ct[184] * t584) +
              t1371 * ct_idx_99_tmp)) -
        ct[47] *
            (((((ct[215] * t1690 + ct[207] * t879) + c_ct_idx_132_tmp * t1011) +
               b_ct_idx_132_tmp * t584) +
              -t602 * t1371) +
             -ct[283] * t1035);
    M_mtrx_sym[3] =
        t855 - ct[23] *
                   ((t637 * 2.0 + t645 * 2.0) - ct[60] * t926_tmp_tmp * 2.0) /
                   2.0;
    t863_tmp =
        ((((((((((((ct_idx_129 + ct_idx_130) +
                   ct[42] * ct[75] * ct_idx_186_tmp * -0.5) +
                  ct[90] * ct_idx_186_tmp) +
                 b_ct_idx_93_tmp * t519) -
                ct_idx_454_tmp * t1447) +
               ct_idx_458_tmp * t1449 / 2.0) +
              ct_idx_95_tmp * t519 * -0.5) +
             ct[34] * t1568) +
            ct[35] * t1569) +
           ct[44] * ((ct[161] * ct_idx_186_tmp + -ct[133] * t519) +
                     ct[67] * t1690)) +
          ct[45] *
              ((ct[159] * ct_idx_186_tmp + ct[133] * t1447) + ct[73] * t1690)) +
         ct[46] * (((ct[161] * t1447 + -ct[67] * t1371) + ct[73] * t584) +
                   ct[159] * t519)) +
        -ct[47] * (((((ct[134] * t1690 + ct[133] * t879) + ct[160] * t1371) +
                     ct[162] * t584) +
                    ct[161] * t1011) +
                   ct[159] * t1035);
    M_mtrx_sym[4] =
        t863_tmp - ct[33] * ((t874 * 2.0 + t926 * 2.0) - t1631_tmp * 2.0) / 2.0;
    t1004 = ((t1451 + t1452) + ct[44] * t584) + ct[45] * t1371;
    M_mtrx_sym[5] =
        t1004 -
        ct[43] * (b_ct_idx_186_tmp_tmp * 2.0 + ct_idx_186_tmp_tmp * 2.0) / 2.0;
    M_mtrx_sym[6] = t839;
    M_mtrx_sym[7] =
        (((((((ct[3] +
               ct[17] * ((ct[96] * ct[99] * 2.0 + ct[119] * ct[121] * 2.0) +
                         ct[120] * ct[122] * 2.0)) +
              ct[27] * ((t703 * t704 * 2.0 + t705 * t711 * 2.0) +
                        t706 * t714 * 2.0)) +
             ct[37] * ((ct_idx_102 * t1112 * 2.0 + t1115 * t1120 * 2.0) +
                       ct_idx_103 * t1122 * 2.0)) +
            -ct[44] * (ct_idx_61_tmp * b_t1192_tmp_tmp * 2.0 -
                       ct_idx_137 * ct_idx_93_tmp * 2.0)) +
           ((((((-ct[34] *
                     (t1115 * t785_tmp * 2.0 - ct_idx_102 * b_t765_tmp * 2.0) +
                 ct[45] * (ct_idx_90 * ct_idx_137 * 2.0 -
                           t1525 * ct_idx_61_tmp * 2.0)) +
                ct[33] * (t785_tmp * t785_tmp)) +
               ct[31] * (b_t765_tmp * b_t765_tmp)) +
              ct[47] * ((b_t1192_tmp_tmp * (t648 * 2.0) + t1525 * t1529 * 2.0) +
                        ct_idx_137 * t1318 * 2.0)) +
             ct[35] * (ct_idx_102 * t755 * 2.0 + ct_idx_103 * t785_tmp * 2.0)) -
            ct[36] * (t1115 * t755 * 2.0 + ct_idx_103 * b_t765_tmp * 2.0))) +
          ((((-ct[46] * (t1525 * ct_idx_93_tmp * 2.0 -
                         ct_idx_90 * b_t1192_tmp_tmp * 2.0) +
              ct[8] * (ct[72] * ct[72])) +
             ct[11] * (ct[65] * ct[65])) +
            ct[23] * (t425 * t425)) +
           ct[21] * (ct[257] * ct[257]))) +
         (((((((((ct[28] * (t755 * t755) + ct[38] * (ct_idx_90 * ct_idx_90)) -
                 ct[16] * (ct[65] * ct[119] * 2.0 + ct[72] * ct[120] * 2.0)) -
                ct[24] * (t425 * t705 * 2.0 + ct[257] * t703 * 2.0)) +
               ct[18] * (t765_tmp * t765_tmp)) -
              ct[25] * (t425 * t706 * 2.0 - t703 * t765_tmp * 2.0)) -
             ct[26] * (ct[257] * t706 * 2.0 + t705 * t765_tmp * 2.0)) +
            ct[41] * (ct_idx_93_tmp * ct_idx_93_tmp)) +
           ct[43] * (ct_idx_61_tmp * ct_idx_61_tmp)) -
          t439 * ct_idx_93_tmp)) +
        ((((((((((t1085_tmp * ct_idx_61_tmp +
                  ct[32] * t785_tmp * t761_tmp_tmp) -
                 ct[42] * ct_idx_61_tmp * t1084_tmp_tmp) -
                ct_idx_439_tmp * t785_tmp) -
               t527 * b_t765_tmp) -
              t869 * ct[77]) +
             ct[14] * ct[65] * ct[96] * 2.0) +
            ct[15] * ct[72] * ct[96] * 2.0) -
           ct[22] * t425 * t491) -
          ct[20] * t425 * t515_tmp) +
         t841 * t515_tmp);
    M_mtrx_sym[8] = t863;
    t1371 =
        ((((((((((((((((((((((((((((-t510 + t515) - ct[24] * t705) -
                                  ct[25] * t706) +
                                 -ct[166] * t785_tmp) +
                                -ct[169] * t785_tmp) +
                               ct[98] * t785_tmp) +
                              ct_idx_132_tmp * t765) +
                             ct_idx_131_tmp * t765) +
                            ct_idx_136_tmp * t755) +
                           ct[87] * t762) +
                          ct_idx_446_tmp * t760 * -0.5) +
                         ct_idx_141_tmp * t760 / 2.0) +
                        ct_idx_62_tmp * ct_idx_61_tmp) +
                       t355 * ct_idx_61_tmp) +
                      ct[189] * ct_idx_61_tmp) +
                     ct[82] * t1084) +
                    ct_idx_73_tmp * t1082 * -0.5) +
                   b_ct_idx_99_tmp * ct_idx_93_tmp) +
                  t518 * ct_idx_90) +
                 ct_idx_100_tmp * ct_idx_93_tmp * -0.5) +
                t439 * ct_idx_99_tmp * -0.5) -
               ct[34] * (((t519_tmp * t785_tmp * 0.02575 +
                           t534_tmp * b_t765_tmp * 0.02575) +
                          t518_tmp * ct_idx_102) +
                         ct[87] * t1115)) +
              ct[35] * (((t518_tmp * t785_tmp * -0.02575 + t783 * -0.02575) +
                         t519_tmp * ct_idx_102) +
                        ct_idx_103 * ct[87])) -
             ct[36] * (((t518_tmp * b_t765_tmp * -0.02575 +
                         t519_tmp * t755 * 0.02575) +
                        t1124) -
                       t1126)) -
            ct[37] * (((((t534_tmp * ct_idx_102 * 0.012875 + ct[55] * t1112) -
                         t1124 * 0.012875) +
                        t1126 * 0.012875) -
                       t519_tmp * t1120 * 0.02575) +
                      t518_tmp * t1122 * 0.02575)) +
           ct[44] *
               (((ct_idx_61_tmp * c_ct_idx_132_tmp + -ct[207] * ct_idx_93_tmp) +
                 ct_idx_137 * ct_idx_99_tmp) +
                ct[82] * b_t1192_tmp_tmp)) +
          -ct[45] * (((ct_idx_90 * ct[207] + -ct[184] * ct_idx_137) -
                      ct[82] * t1525) +
                     ct[283] * ct_idx_61_tmp)) +
         -ct[46] * (((-ct[184] * b_t1192_tmp_tmp + t1525 * ct_idx_99_tmp) +
                     ct[283] * ct_idx_93_tmp) +
                    ct_idx_90 * c_ct_idx_132_tmp)) +
        -ct[47] *
            (((((b_ct_idx_132_tmp * b_t1192_tmp_tmp + c_ct_idx_132_tmp * t648) -
                ct[283] * t1529) -
               t602 * t1525) +
              ct[207] * t1318) +
             ct[215] * ct_idx_137);
    M_mtrx_sym[9] = t1371 + ct[23] * (ct[129] * 2.0 - ct[170] * 2.0) / 2.0;
    t818 = ((((((((((((t762 - ct_idx_439) + -ct[84] * ct_idx_61_tmp) +
                     ct_idx_63_tmp * ct_idx_61_tmp / 2.0) +
                    b_ct_idx_93_tmp * ct_idx_93_tmp) +
                   ct_idx_454_tmp * ct_idx_90) +
                  ct_idx_95_tmp * ct_idx_93_tmp * -0.5) -
                 ct_idx_458_tmp * t1082 / 2.0) -
                ct[34] * t1115) +
               ct[35] * ct_idx_103) -
              ct[44] * ((-ct[161] * ct_idx_61_tmp + ct[133] * ct_idx_93_tmp) +
                        -ct[67] * ct_idx_137)) -
             ct[45] * ((-ct[159] * ct_idx_61_tmp + ct_idx_90 * ct[133]) +
                       -ct[73] * ct_idx_137)) -
            ct[46] * (((ct_idx_90 * ct[161] + -ct[73] * b_t1192_tmp_tmp) +
                       ct[67] * t1525) -
                      ct[159] * ct_idx_93_tmp)) +
           -ct[47] *
               (((((ct[160] * t1525 + ct[159] * t1529) + ct[134] * ct_idx_137) +
                  ct[133] * t1318) +
                 ct[162] * b_t1192_tmp_tmp) +
                ct[161] * t648);
    M_mtrx_sym[10] =
        t818 + ct[33] * ((t435 * 2.0 - t534 * 2.0) + t785_tmp_tmp * 2.0) / 2.0;
    t810 = ((t1084 - t1085) + ct[44] * b_t1192_tmp_tmp) + ct[45] * t1525;
    M_mtrx_sym[11] =
        t810 - ct[43] * (t783 * 2.0 + ct_idx_61_tmp_tmp * 2.0) / 2.0;
    M_mtrx_sym[12] = t870;
    M_mtrx_sym[13] = t863;
    M_mtrx_sym[14] =
        (((((ct[13] + ct[27] * ((ct[56] * ct[61] * 0.0070476025 +
                                 ct[61] * ct[74] * 0.0070476025) +
                                ct[60] * ct[60] * 0.0070476025)) -
            ct[24] * (ct[128] * 0.1679 - ct[61] * ct[64] * 0.1679)) -
           ct[25] * (ct[127] * 0.1679 + ct[61] * ct[68] * 0.1679)) +
          ct[47] * ((t1059 * ct_idx_81 * 2.0 + t1152 * ct_idx_112 * 2.0) -
                    t1153 * t815 * 2.0)) +
         ((((((((((((ct[37] *
                         ((ct[302] * ct[305] * 2.0 + ct[303] * t679 * 2.0) +
                          t824 * t1303 * 2.0) +
                     ct[34] *
                         (ct[235] * ct[302] * 2.0 + ct[246] * t824 * 2.0)) +
                    ct[35] * (ct[235] * ct[303] * 2.0 - t460 * t824 * 2.0)) +
                   ct[28] * (t460 * t460)) +
                  ct[33] * (ct[235] * ct[235])) +
                 ct[31] * (ct[246] * ct[246])) +
                ct[43] * (t690 * t690)) +
               ct[38] * (t743 * t743)) +
              ct[41] * (t742 * t742)) -
             ct[26] * (ct[56] * ct[58] * ct[59] * 0.1679 +
                       ct[58] * ct[74] * ct[59] * 0.1679)) -
            ct[36] * (ct[246] * ct[303] * 2.0 + t460 * ct[302] * 2.0)) +
           ct[44] * (t1059 * t742 * 2.0 - t1152 * t690 * 2.0)) -
          ct[45] * (t1059 * t743 * 2.0 + t1153 * t690 * 2.0))) +
        ((((((((((((-ct[46] * (t1153 * t742 * 2.0 + t1152 * t743 * 2.0) +
                    ct[23] * (ct[57] * ct[57]) * ct[61]) +
                   ct[18] * ct[56] * ct[62]) +
                  ct[21] * ct[74] * ct[62]) +
                 ct_idx_154_tmp * t467) +
                ct[32] * ct[235] * ct[248]) +
               ct_idx_160_tmp * t467) -
              t913 * t745) -
             ct[42] * t690 * t744) +
            t1220 * t745) -
           t585 * ct[315] * ct[62] * 2.0) +
          ct[20] * ct[58] * ct[59] * ct[64] * 2.0) -
         ct[22] * ct[58] * ct[59] * ct[68] * 2.0);
    M_mtrx_sym[15] = t785;
    t870 = ((((((((((((-t468 - t471) - ct[304]) - ct[35] * ct[303]) +
                    ct[84] * t690) +
                   ct_idx_63_tmp * t690 * -0.5) -
                  b_ct_idx_93_tmp * t742) +
                 ct_idx_454_tmp * t743) +
                ct_idx_95_tmp * t742 / 2.0) -
               ct_idx_458_tmp * t745 / 2.0) -
              ct[44] * ((ct[161] * t690 - ct[133] * t742) + ct[67] * t1059)) -
             ct[45] * ((ct[159] * t690 + ct[133] * t743) + ct[73] * t1059)) -
            ct[46] * (((ct[159] * t742 + ct[161] * t743) + ct[73] * t1152) -
                      ct[67] * t1153)) +
           ct[47] *
               (((((ct[134] * t1059 + ct_idx_81 * ct[133]) + ct[162] * t1152) +
                  ct[160] * t1153) +
                 ct_idx_112 * ct[161]) +
                -ct[159] * t815);
    M_mtrx_sym[16] =
        t870 - ct[33] * ((ct[109] * 2.0 + ct[114] * 2.0) - ct[140] * 2.0) / 2.0;
    t839 = ((-t747 - ct_idx_433) - ct[45] * t1153) - ct[44] * t1152;
    M_mtrx_sym[17] = t839 + ct[43] * (ct[253] * 2.0 - t507 * 2.0) / 2.0;
    M_mtrx_sym[18] = t855 - ct[23] * t868;
    M_mtrx_sym[19] = t1371 + t1017;
    M_mtrx_sym[20] = t785;
    t855 = ct[66] * ct[107];
    t1371 = ct[80] * ct[107];
    M_mtrx_sym[21] =
        (((((((ct[23] + ct[37] * ((t855 * 0.0006630625 + t1371 * 0.0006630625) +
                                  ct[102] * ct[107] * 0.0006630625)) -
              ct[30] * ct[313] * 2.0) -
             ct[32] * ct[311] * 2.0) +
            ct[87] * ct[98]) -
           ct[34] * (ct[313] * 0.0515 - ct[107] * ct[117] * 0.0515)) +
          ct[47] * ((ct[207] * ct[215] * 2.0 + ct[283] * t602 * 2.0) +
                    c_ct_idx_132_tmp * b_ct_idx_132_tmp * 2.0)) -
         ct[44] * (ct[207] * ct_idx_99_tmp * 2.0 +
                   ct[82] * c_ct_idx_132_tmp * 2.0)) +
        ((((((((((((-ct[46] * (ct[283] * ct_idx_99_tmp * 2.0 +
                               ct[184] * c_ct_idx_132_tmp * 2.0) +
                    ct[43] * (ct[82] * ct[82])) +
                   ct[38] * (ct[184] * ct[184])) -
                  ct[45] * (ct[184] * ct[207] * 2.0 - ct[82] * ct[283] * 2.0)) -
                 ct[36] * (t855 * 0.0515 + t1371 * 0.0515)) +
                ct[41] * (ct_idx_99_tmp * ct_idx_99_tmp)) -
               ct[35] * (ct[311] * 0.0515 + ct[50] * 0.0515)) +
              ct[28] * ct[80] * ct[107]) +
             ct_idx_132_tmp * ct[205] * 2.0) +
            ct[31] * ct[66] * ct[107]) -
           ct_idx_73_tmp * t342) +
          t813 * ((ct[51] * -2.0 + ct[108]) + ct_idx_99_tmp_tmp * 2.0)) -
         ct_idx_100_tmp * ct_idx_99_tmp);
    M_mtrx_sym[22] = ct_idx_132;
    t855 = ((-t355 + ct[190]) + ct[45] * ct[283]) + ct[284];
    M_mtrx_sym[23] = t855 + ct[43] * (ct[205] * 2.0 - ct[50] * 2.0) / 2.0;
    M_mtrx_sym[24] = t863_tmp - ct[33] * t1174;
    M_mtrx_sym[25] = t818 + ct[33] * t785_tmp;
    M_mtrx_sym[26] = t870 - ct_idx_151_tmp;
    M_mtrx_sym[27] = ct_idx_132;
    M_mtrx_sym[28] =
        ((((((ct[33] +
              ct[47] * ((ct[133] * ct[134] * 2.0 + ct[159] * ct[160] * 2.0) +
                        ct[161] * ct[162] * 2.0)) +
             ct[38] * (ct[73] * ct[73])) +
            ct[41] * (ct[67] * ct[67])) +
           ct[46] * (ct[67] * ct[159] * 2.0 - ct[73] * ct[161] * 2.0)) -
          ct_idx_458_tmp * ct[78]) -
         ct[44] * ct[67] * ct[133] * 2.0) -
        ct[45] * ct[73] * ct[133] * 2.0;
    M_mtrx_sym[29] = ct_idx_353;
    M_mtrx_sym[30] = t1004 - t927;
    M_mtrx_sym[31] = t810 - ct[43] * ct_idx_61_tmp;
    M_mtrx_sym[32] = t839 + ct[43] * t690;
    M_mtrx_sym[33] = t855 + t831;
    M_mtrx_sym[34] = ct_idx_353;
    M_mtrx_sym[35] = ct[43];
}

/*
 * M_mtrx_fcn
 *     M_mtrx_sym = M_mtrx_fcn(IN1,IN2)
 *
 * Arguments    : const double in1[6]
 *                const double in2[66]
 *                double M_mtrx_sym[36]
 * Return Type  : void
 */
void M_mtrx_fcn(const double in1[6], const double in2[66],
                double M_mtrx_sym[36])
{
    double b_in2[318];
    double t10;
    double t100;
    double t101;
    double t108;
    double t109;
    double t11;
    double t127;
    double t128;
    double t129;
    double t130;
    double t131;
    double t132;
    double t133;
    double t135;
    double t136;
    double t137;
    double t139;
    double t140;
    double t142;
    double t143;
    double t148;
    double t149;
    double t152;
    double t155;
    double t156;
    double t157;
    double t166;
    double t171;
    double t175;
    double t176;
    double t18;
    double t189;
    double t192;
    double t195;
    double t197;
    double t197_tmp;
    double t2;
    double t200;
    double t201;
    double t204;
    double t21;
    double t214;
    double t218;
    double t22;
    double t222;
    double t223;
    double t226;
    double t23;
    double t230;
    double t232;
    double t234;
    double t235;
    double t239;
    double t24;
    double t244;
    double t245;
    double t246;
    double t25;
    double t253;
    double t26;
    double t261;
    double t267;
    double t269;
    double t27;
    double t271;
    double t272;
    double t28;
    double t29;
    double t3;
    double t30;
    double t300;
    double t302;
    double t31;
    double t314;
    double t32;
    double t320;
    double t321;
    double t326;
    double t327;
    double t328;
    double t331;
    double t332;
    double t333;
    double t343;
    double t344;
    double t345;
    double t347;
    double t348;
    double t359;
    double t36;
    double t361;
    double t369;
    double t37;
    double t370;
    double t372;
    double t373;
    double t374;
    double t375;
    double t38;
    double t381;
    double t39;
    double t4;
    double t40;
    double t41;
    double t426;
    double t427;
    double t428;
    double t436;
    double t445;
    double t490;
    double t5;
    double t6;
    double t62_tmp;
    double t63;
    double t7;
    double t72;
    double t73;
    double t76;
    double t77;
    double t8;
    double t9;
    /*     This function was generated by the Symbolic Math Toolbox
     * version 24.1. */
    /*     2025-10-15 11:29:30 */
    t2 = cos(in1[1]);
    t3 = cos(in1[2]);
    t4 = cos(in1[3]);
    t5 = cos(in1[4]);
    t6 = cos(in1[5]);
    t7 = sin(in1[1]);
    t8 = sin(in1[2]);
    t9 = sin(in1[3]);
    t10 = sin(in1[4]);
    t11 = sin(in1[5]);
    t18 = in2[45] * -3.6732051033465739E-6;
    t21 = t3 * -3.6732051033465739E-6;
    t22 = t5 * -3.6732051033465739E-6;
    t23 = t6 * -3.6732051033465739E-6;
    t24 = t8 * -3.6732051033465739E-6;
    t25 = t3 * 0.99999999999325373;
    t26 = t10 * -3.6732051033465739E-6;
    t27 = t11 * -3.6732051033465739E-6;
    t28 = t6 * 0.99999999999325373;
    t29 = t8 * 0.99999999999325373;
    t30 = t11 * 0.99999999999325373;
    t31 = in2[44] * t5 * 0.99999999999325373;
    t32 = in2[42] * t10 * 0.99999999999325373;
    t62_tmp = t5 * 0.99999999999325373 * 0.0914;
    t63 = t10 * 0.99999999999325373 * 0.0914;
    t128 = t4 * 3.2679489653813835E-7;
    t131 = t2 * 0.9999999999999466;
    t132 = t9 * 3.2679489653813835E-7;
    t133 = t7 * 0.9999999999999466;
    t135 = in2[34] * t9 * 0.9999999999999466;
    t149 = t2 * 3.2679489653813835E-7 * 0.165;
    t157 = t9 * 0.9999999999999466 * 0.02575;
    t166 = t7 * 3.2679489653813835E-7 * 0.0313;
    t171 = t4 * -0.99999999999994 * 0.08395;
    t175 = t9 * -0.99999999999994 * 0.08395;
    t192 = t5 * 0.99999999999325373 * -0.090162;
    t195 = t5 * t9 * 0.99999999999325373 * 0.9999999999999466;
    t197_tmp = t9 * t10;
    t197 = t197_tmp * 0.99999999999325373 * 0.9999999999999466;
    t214 =
        t9 * -3.6732051033465739E-6 * 0.99999999999325373 * 0.9999999999999466;
    t234 = t4 * 0.9999999999999466 * -3.4641020661939347E-7;
    t235 = t9 * 0.9999999999999466 * -3.4641020661939347E-7;
    t267 = t5 * 0.99999999999325373 * 3.2679489653813835E-7 * -0.99999999999994;
    t269 =
        t10 * 0.99999999999325373 * 3.2679489653813835E-7 * -0.99999999999994;
    t36 = t30 * 2.0;
    t37 = t3 * 0.99999999998650746;
    t38 = t5 * 0.99999999998650746;
    t39 = t6 * 0.99999999998650746;
    t40 = t8 * 0.99999999998650746;
    t41 = t11 * 0.99999999998650746;
    t72 = 0.99999999999325373 * t21;
    t73 = 0.99999999999325373 * t22;
    t3 = 0.99999999999325373 * t23;
    t76 = 0.99999999999325373 * t24;
    t77 = 0.99999999999325373 * t26;
    t100 = 0.99999999998650746 * t26;
    t101 = t22 * t28;
    t176 = t133 * 0.02805;
    t8 = t2 * 0.9999999999998932 * 0.0313;
    t201 = t4 * 0.99999999998650746 * 0.9999999999999466;
    t232 = 3.2679489653813835E-7 * t131;
    t11 = 3.2679489653813835E-7 * t133;
    t314 = (t10 * 0.99999999999325373 * 0.045 - 9.4585031410536173E-8) + t192;
    t320 = t5 * 0.99999999999325373 * (t157 - -2.9081136845698081E-8);
    t327 = t132 + t234;
    t108 = t21 + t40;
    t109 = t23 + t41;
    t127 = t26 + t73;
    t129 = t29 + t72;
    t130 = t30 + t3;
    t218 = -3.6732051033217936E-6 * (t62_tmp - 1.6529422965059582E-7);
    t261 = 0.99999999999325373 * (t63 - 3.3118351852793381E-7);
    t271 = t23 * (t63 - 3.3118351852793381E-7);
    t272 = t27 * (t63 - 3.3118351852793381E-7);
    t26 = t175 - t9 * 0.9999999999999466 * 0.28435;
    t326 = t131 + t11;
    t332 = t128 - t235;
    t374 = (-t149 + t166) + t8;
    t6 = (t171 - t4 * 0.9999999999999466 * 0.28435) - 8.4149685858565577E-9;
    t428 = (t201 + t214) - 1.2003846817114336E-12;
    t136 = t22 - t77;
    t137 = t25 + 0.99999999999325373 * -t24;
    t139 = t28 + 0.99999999999325373 * -t27;
    t140 = t36 + t3 * 2.0;
    t148 = t10 * 0.99999999999325373 * t109;
    t152 = t38 - t100;
    t189 = -3.6732051033465739E-6 * t130;
    t222 = (t24 * 0.14 + t21 * 0.0313) + t40 * 0.0313;
    t223 = (t21 * 0.14 - t24 * 0.0313) + t37 * 0.0313;
    t226 = (t21 * 0.07 - t24 * 0.01565) + t37 * 0.01565;
    t230 = t109 * (t62_tmp - 1.6529422965059582E-7);
    t239 = -3.4641020661939347E-7 * t129;
    t244 = t4 * 0.9999999999999466 * t127;
    t245 = t4 * -0.99999999999994 * t129;
    t246 = t9 * -0.99999999999994 * t129;
    t300 = (t23 * 0.045 + t41 * 0.045) - t27 * 0.090162;
    t3 = (t29 * 0.08395 + t72 * 0.08395) + 0.14000011497037529;
    t328 = 0.99999999998650746 * t26;
    t331 = t133 - t232;
    t359 = -3.6732051033217936E-6 * t326;
    t361 = t127 * t26;
    t370 = t130 * t314;
    t373 = t108 * t326;
    t375 = (t7 * 3.2679489653813835E-7 * 0.165 +
            t2 * 3.2679489653813835E-7 * 0.0313) -
           t7 * 0.9999999999998932 * 0.0313;
    t381 = t218 + t261;
    t426 = t5 * 0.99999999999325373 * t6;
    t427 = t10 * 0.99999999999325373 * t6;
    t436 = -3.6732051033217936E-6 * t6;
    t490 = t130 * t428;
    t63 =
        (((t149 - t166) + t131 * 0.02805) - t8) + 3.2679489653813835E-7 * t176;
    t11 = (((-(t133 * 0.14) + t131 * 0.0313) + t232 * 0.14) + t11 * 0.0313) +
          0.16500001022867147;
    t142 = t25 * 2.0 - t76 * 2.0;
    t143 = t28 * 2.0 - 0.99999999999325373 * t27 * 2.0;
    t155 = in2[24] * (t29 * 2.0 + t72 * 2.0) / 2.0;
    t156 = in2[54] * t140 / 2.0;
    t200 = t189 * 2.0;
    t204 = -3.6732051033465739E-6 * t139;
    t253 = t4 * 0.9999999999999466 * t136;
    t302 = (t27 * 0.045 - t39 * 0.045) + t23 * 0.090162;
    t321 = -0.99999999999988665 * t137;
    t333 = t328 / 2.0;
    t343 = (-(t25 * 0.02805) + t76 * 0.02805) + t223;
    t344 = 0.99999999999325373 * t331;
    t345 = (-(t29 * 0.02805) - t72 * 0.02805) + t222;
    t347 = t21 * t331;
    t348 = t24 * t331;
    t369 = t136 * t26;
    t372 = t139 * t314;
    t6 = t4 * 0.9999999999999466 * t3;
    t8 = t9 * 0.9999999999999466 * t3;
    t314 = t218 / 2.0 + t261 / 2.0;
    t218 = t137 * t327;
    t261 = t137 * t332;
    t72 = -3.2679489653811876E-7 * t3;
    t445 = t436 / 2.0;
    t25 = (-t197 + t244) + t267;
    t29 = t139 * t428;
    t133 = (t176 + t232 * -0.02805) + t375;
    t28 = 0.99999999999325373 * t63;
    t76 = t24 * t63;
    t166 = t21 * t63;
    t131 = t129 * t11;
    t63 = t137 * t11;
    t149 = (t320 + t361) + t427;
    b_in2[0] = in2[10];
    b_in2[1] = in2[11];
    b_in2[2] = in2[13];
    memcpy(&b_in2[3], &in2[15], 44U * sizeof(double));
    b_in2[47] = in2[59];
    b_in2[48] = in2[5];
    b_in2[49] = t10;
    b_in2[50] = t100;
    b_in2[51] = t101;
    b_in2[52] = t22 * t36;
    b_in2[53] = -1.652942296494807E-7;
    b_in2[54] = t109;
    b_in2[55] = -9.4585031410536173E-8;
    b_in2[56] = t4 * t4;
    b_in2[57] = 3.2679489653813835E-7;
    b_in2[58] = -0.99999999999994;
    b_in2[59] = 0.9999999999999466;
    b_in2[60] = -3.4641020661939347E-7;
    b_in2[61] = 0.9999999999998801;
    b_in2[62] = 0.9999999999998932;
    b_in2[63] = t127;
    b_in2[64] = t128;
    b_in2[65] = t129;
    b_in2[66] = t5 * t5;
    b_in2[67] = t130;
    b_in2[68] = t132;
    b_in2[69] = in2[32] * t4 * 0.9999999999999466;
    b_in2[70] = t135;
    b_in2[71] = t136;
    b_in2[72] = t137;
    b_in2[73] = t139;
    b_in2[74] = t9 * t9;
    b_in2[75] = t140;
    b_in2[76] = t62_tmp - 1.6529422965059582E-7;
    b_in2[77] = t142;
    b_in2[78] = t143;
    b_in2[79] = -t135;
    b_in2[80] = t10 * t10;
    b_in2[81] = t148 * 2.0;
    b_in2[82] = t152;
    b_in2[83] = t155;
    b_in2[84] = t156;
    b_in2[85] = t157;
    b_in2[86] = -2.9081136845698081E-8;
    b_in2[87] = -3.6732051033465739E-6;
    b_in2[88] = in2[22] * t142 / 2.0;
    b_in2[89] = -t155;
    t26 = in2[52] * t143;
    b_in2[90] = t26 / 2.0;
    b_in2[91] = -3.3118351852793381E-7;
    b_in2[92] = 0.090161999999391743;
    b_in2[93] = 0.99999999999325373;
    b_in2[94] = t171;
    b_in2[95] = t175;
    b_in2[96] = 0.14000011497037529;
    b_in2[97] = t5 * 0.090161999999391743;
    b_in2[98] = t18;
    b_in2[99] = 0.070000057485187644;
    b_in2[100] = in2[36] * t171;
    b_in2[101] = t189;
    b_in2[102] = 1.3492435731251315E-11;
    b_in2[103] = t192;
    b_in2[104] = t195;
    b_in2[105] = in2[37] * t9 * -0.99999999999994 * -0.08395;
    b_in2[106] = t2;
    b_in2[107] = 0.99999999998650746;
    b_in2[108] = t200;
    b_in2[109] = t201;
    b_in2[110] = t195 * 2.0;
    b_in2[111] = t204 * 2.0;
    b_in2[112] = t195 * 0.045;
    b_in2[113] = -(t201 * 0.045);
    b_in2[114] = t214;
    b_in2[115] = t195 * 0.06565;
    b_in2[116] = -(t197 * 0.06565);
    b_in2[117] = t22;
    b_in2[118] = t214 * -0.045;
    b_in2[119] = t222;
    b_in2[120] = t223;
    b_in2[121] = (t24 * 0.07 + t21 * 0.01565) + t40 * 0.01565;
    b_in2[122] = t226;
    b_in2[123] = in2[26] * t223;
    b_in2[124] = -(in2[27] * t222);
    b_in2[125] = t23;
    b_in2[126] = in2[35] * 3.2679489653813835E-7 * -0.99999999999994;
    b_in2[127] = t234;
    b_in2[128] = t235;
    b_in2[129] = t239;
    b_in2[130] = t24;
    b_in2[131] = -(t230 / 2.0);
    b_in2[132] = t246;
    b_in2[133] = 0.090161834705162092;
    b_in2[134] = 0.045080917352581046;
    b_in2[135] = t245 * 2.0;
    b_in2[136] = t246 * 2.0;
    b_in2[137] = -8.4149685858565577E-9;
    b_in2[138] = t253 * 2.0;
    b_in2[139] = -t253;
    b_in2[140] = 1.2003846817114336E-12;
    b_in2[141] = -(t239 * 0.02575);
    b_in2[142] = t269;
    b_in2[143] = t27;
    b_in2[144] = t271;
    b_in2[145] = t253 * 0.045;
    b_in2[146] = t245 * 0.02575;
    b_in2[147] = t269 * 2.0;
    b_in2[148] = t244 * 0.06565;
    b_in2[149] = t271 / 2.0;
    b_in2[150] = t272 / 2.0;
    b_in2[151] = t197_tmp * 0.9999999999999466 * 0.090161999999391743;
    b_in2[152] = -(t245 * 0.28435);
    b_in2[153] = -(t246 * 0.28435);
    b_in2[154] = -(t201 * 0.090162);
    b_in2[155] = 5.4017310677014507E-14;
    b_in2[156] = t253 * 0.06565;
    b_in2[157] = t152 * t156;
    b_in2[158] = t26 * t152 * -0.5;
    b_in2[159] = t300;
    b_in2[160] = (t23 * 0.0225 + t41 * 0.0225) - t27 * 0.045081;
    b_in2[161] = t302;
    b_in2[162] = (t27 * 0.0225 - t39 * 0.0225) + t23 * 0.045081;
    b_in2[163] = t267 * 0.06565;
    b_in2[164] = t269 * 0.06565;
    b_in2[165] = in2[56] * t302;
    b_in2[166] = t31;
    b_in2[167] = t214 * -0.090162;
    b_in2[168] = -(in2[57] * t300);
    b_in2[169] = t32;
    b_in2[170] = t321;
    b_in2[171] =
        t10 * 0.99999999999325373 * (t157 - -2.9081136845698081E-8) * -0.5;
    b_in2[172] = t244 * 0.090162;
    b_in2[173] = t326;
    b_in2[174] = t327;
    b_in2[175] = t328;
    b_in2[176] = -t321;
    b_in2[177] = in2[12] * t326;
    b_in2[178] = t331;
    b_in2[179] = t332;
    b_in2[180] = t333;
    b_in2[181] = 1.0822908367246626E-13;
    b_in2[182] = -(in2[14] * t331);
    b_in2[183] = t321 * 0.02575;
    b_in2[184] = (t22 * t30 + t148) + t204;
    b_in2[185] = 0.16500001022867147;
    b_in2[186] = t347;
    b_in2[187] = t347 * 2.0;
    b_in2[188] = t348 * 2.0;
    t11 = t27 - t39;
    t3 = t10 * 0.99999999999325373 * t11;
    t26 = in2[54] * ((t101 * -2.0 + t200) + t3 * 2.0);
    b_in2[189] = t26 * -0.5;
    b_in2[190] = t26 / 2.0;
    b_in2[191] = t344 * 0.08395;
    b_in2[192] = t347 * 0.02805;
    b_in2[193] = t347 * 0.08395;
    b_in2[194] = t152 * t302;
    b_in2[195] = t348 * 0.02805;
    b_in2[196] = t152 * t300;
    b_in2[197] = t37;
    b_in2[198] = t370;
    b_in2[199] = t374;
    b_in2[200] = t375;
    b_in2[201] = t369 / 2.0;
    b_in2[202] = t370 / 2.0;
    b_in2[203] = t373 * 2.0;
    b_in2[204] = t6;
    b_in2[205] = t38;
    b_in2[206] = t8;
    b_in2[207] = t381;
    b_in2[208] = t359 * 0.08395;
    b_in2[209] = t372 / 2.0;
    b_in2[210] = in2[16] * t374;
    b_in2[211] = in2[17] * t375;
    b_in2[212] = t6 / 2.0;
    b_in2[213] = t8 / 2.0;
    b_in2[214] = t39;
    b_in2[215] = t314;
    b_in2[216] = t373 * 0.02805;
    b_in2[217] = t218 * 2.0;
    b_in2[218] = t261;
    t26 = t24 - t37;
    t8 = t326 * t26;
    b_in2[219] = t8 * 0.02805;
    b_in2[220] = t261 * 2.0;
    b_in2[221] = t218 * 0.02575;
    b_in2[222] = t4;
    b_in2[223] = -3.4641020661939347E-7 * t343;
    b_in2[224] = t218 * 0.28435;
    b_in2[225] = t4 * -0.99999999999994 * t343;
    b_in2[226] = t9 * -0.99999999999994 * t343;
    b_in2[227] = t261 * 0.28435;
    t6 = (-t101 + t189) + t3;
    b_in2[228] = in2[53] * t130 * t6;
    b_in2[229] = in2[51] * t143 * t6 * -0.5;
    b_in2[230] = t72;
    b_in2[231] = -t72;
    b_in2[232] = t72 / 2.0;
    b_in2[233] = t130 * t381;
    b_in2[234] = t139 * t381;
    b_in2[235] = t428;
    b_in2[236] = t426 / 2.0;
    b_in2[237] = -t27;
    b_in2[238] = -0.99999999999988665 * t345;
    b_in2[239] = t436;
    b_in2[240] = 0.090161834705162092 * t6;
    b_in2[241] = t445;
    b_in2[242] = -t31;
    b_in2[243] = t31 * t428;
    b_in2[244] = t32 * t428;
    b_in2[245] = -(t18 * t428);
    b_in2[246] = t25;
    b_in2[247] = -t32;
    b_in2[248] = (-(t197 * 2.0) + t244 * 2.0) + t267 * 2.0;
    b_in2[249] = t10 * 0.99999999999325373 * t428 * 0.02575;
    b_in2[250] = -(t5 * 0.99999999999325373 * t428 * 0.02575);
    b_in2[251] = -t300 * t6;
    b_in2[252] = 0.045080917352581046 * t381;
    b_in2[253] = 0.99999999999325373 * t25;
    b_in2[254] = t23 * t25;
    b_in2[255] = t27 * t25;
    b_in2[256] = 0.090161834705162092 * t314;
    b_in2[257] = t245 + t218;
    b_in2[258] = t490;
    b_in2[259] = t490 * 2.0;
    b_in2[260] = t5;
    b_in2[261] = -1.6529422965059582E-7;
    b_in2[262] = t29 * 2.0;
    b_in2[263] = -t29;
    b_in2[264] = t327 * t345;
    b_in2[265] = t332 * t345;
    t6 = t246 - t261;
    t3 = 0.99999999998650746 * t6;
    b_in2[266] = t3 * -0.045;
    b_in2[267] = t344 + t359;
    b_in2[268] = -in2[30] * t4 * 0.9999999999999466 * t6;
    b_in2[269] = -t24 * t331 + t373;
    b_in2[270] = t133;
    b_in2[271] = t136 * t6;
    b_in2[272] = 2.9081136845698081E-8 * t6;
    b_in2[273] = -t171 * t6;
    b_in2[274] = t127 * t6 * 0.06565;
    b_in2[275] = t3 * 0.090162;
    b_in2[276] = t28;
    b_in2[277] = t76;
    b_in2[278] = t166;
    b_in2[279] = t28 / 2.0;
    b_in2[280] = t76 / 2.0;
    b_in2[281] = t166 / 2.0;
    b_in2[282] = -3.6732051033217936E-6 * t133;
    b_in2[283] = (-t230 + t272) + t372;
    t6 = (t271 - t370) + (t62_tmp - 1.6529422965059582E-7) * t11;
    b_in2[284] = -in2[56] * t6;
    b_in2[285] = t108 * t133;
    b_in2[286] = -t133 * t26;
    t8 += t347;
    b_in2[287] = 3.4641020661939347E-7 * t8;
    b_in2[288] = in2[23] * t129 * t8;
    b_in2[289] = -3.4641020661939347E-7 * t8 * -0.02575;
    b_in2[290] = in2[21] * t142 * t8 * -0.5;
    t3 = t4 * -0.99999999999994 * t8;
    b_in2[291] = t3 * 0.02575;
    b_in2[292] = t9 * -0.99999999999994 * t8 * -0.28435;
    b_in2[293] = -0.14000011497037529 * t8;
    b_in2[294] = t3 * 0.28435;
    b_in2[295] = t139 * t6;
    b_in2[296] = t131;
    b_in2[297] = t63;
    b_in2[298] = -t131;
    b_in2[299] = t131 / 2.0;
    b_in2[300] = t63 / 2.0;
    t3 = -3.6732051033465739E-6 * (t157 - -2.9081136845698081E-8);
    t8 = (-t328 + t436) + t3;
    b_in2[301] = t10 * 0.99999999999325373 * t8;
    b_in2[302] = t149;
    b_in2[303] =
        (-t10 * 0.99999999999325373 * (t157 - -2.9081136845698081E-8) + t369) +
        t426;
    b_in2[304] = in2[46] * t149;
    b_in2[305] = (t320 / 2.0 + t361 / 2.0) + t427 / 2.0;
    b_in2[306] = -3.6732051033217936E-6 * t8 * 0.012875;
    b_in2[307] = -3.6732051033465739E-6 * t149;
    b_in2[308] = t10 * 0.99999999999325373 * t149;
    b_in2[309] = -3.6732051033217936E-6 * ((-t333 + t445) + t3 / 2.0) * 0.02575;
    b_in2[310] = t7;
    b_in2[311] = t73;
    b_in2[312] = -(in2[47] * t5 * 0.99999999999325373 * 0.02575);
    b_in2[313] = t77;
    b_in2[314] = -(in2[46] * t10 * 0.99999999999325373 * 0.02575);
    b_in2[315] = t9;
    t26 = (-t166 + t131) + t133 * t26;
    b_in2[316] = in2[26] * t26;
    b_in2[317] = -t226 * t26;
    ft_1(b_in2, M_mtrx_sym);
}

/*
 * File trailer for M_mtrx_fcn.c
 *
 * [EOF]
 */
