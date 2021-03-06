
#include "ACController.h"

/*****************Config bit settings****************/
_FOSC(0xFFFF & XT_PLL16);//XT_PLL4); // Use XT with external crystal from 4MHz to 10MHz.  FRM Pg. 178
// nominal clock is 128kHz.  The counter is 1 byte. 
//#define STANDARD_THROTTLE
//#ifdef DEBUG
	_FWDT(WDT_OFF);
//#else
//	_FWDT(WDT_ON & WDTPSA_64 & WDTPSB_8); // See Pg. 709 in F.R.M.  Timeout in 1 second or so.  128000 / 64 / 8 / 256
//#endif

_FBORPOR(0xFFFF & BORV_27 & PWRT_64 & MCLR_EN & PWMxL_ACT_HI & PWMxH_ACT_HI); // Brown Out voltage set to 2v.  Power up time of 64 ms. MCLR is enabled.
// PWMxL_ACT_HI means PDC1 = 0 would mean PWM1L is 0 volts.  Just the opposite of how I always thought it worked.  haha.
// PWMxH_ACT_HI means, in complementary mode, that PDC1 = 0 would mean PWM1H is 0v, and PWM1L is 5v. 
_FGS(CODE_PROT_OFF);

//25,400,624,972,5
#ifndef PAULS_MOTOR
const SavedValuesStruct savedValuesDefault = {
	AC_INDUCTION_MOTOR,DEFAULT_KP,DEFAULT_KI,DEFAULT_CURRENT_SENSOR_AMPS_PER_VOLT,390,490,510,610,5,MAX_BATTERY_AMPS,MAX_BATTERY_AMPS_REGEN,MAX_MOTOR_AMPS,MAX_MOTOR_AMPS,DEFAULT_PRECHARGE_TIME,{0},0
};

volatile SavedValuesStruct savedValues = {
	AC_INDUCTION_MOTOR,DEFAULT_KP,DEFAULT_KI,DEFAULT_CURRENT_SENSOR_AMPS_PER_VOLT,390,490,510,610,5,MAX_BATTERY_AMPS,MAX_BATTERY_AMPS_REGEN,MAX_MOTOR_AMPS,MAX_MOTOR_AMPS,DEFAULT_PRECHARGE_TIME,{0},0
};

const SavedValuesStruct2 savedValuesDefault2 = {
	0, DEFAULT_ROTOR_TIME_CONSTANT_INDEX,DEFAULT_NUM_POLE_PAIRS,DEFAULT_MAX_MECHANICAL_RPM,DEFAULT_THROTTLE_TYPE, DEFAULT_ENCODER_TICKS,DEFAULT_DATA_TO_DISPLAY_SET1, DEFAULT_DATA_TO_DISPLAY_SET2,0,{0,0,0,0,0,0}, 0
};
volatile SavedValuesStruct2 savedValues2 = {
	0, DEFAULT_ROTOR_TIME_CONSTANT_INDEX,DEFAULT_NUM_POLE_PAIRS,DEFAULT_MAX_MECHANICAL_RPM,DEFAULT_THROTTLE_TYPE, DEFAULT_ENCODER_TICKS,DEFAULT_DATA_TO_DISPLAY_SET1, DEFAULT_DATA_TO_DISPLAY_SET2,0,{0,0,0,0,0,0}, 0
};
#else
const SavedValuesStruct savedValuesDefault = {
	DEFAULT_MOTOR_TYPE,DEFAULT_KP,DEFAULT_KI,DEFAULT_CURRENT_SENSOR_AMPS_PER_VOLT,25,400,624,972,5,MAX_BATTERY_AMPS,MAX_BATTERY_AMPS_REGEN,MAX_MOTOR_AMPS,MAX_MOTOR_AMPS,DEFAULT_PRECHARGE_TIME,{0},0
};

volatile SavedValuesStruct savedValues = {
	DEFAULT_MOTOR_TYPE,DEFAULT_KP,DEFAULT_KI,DEFAULT_CURRENT_SENSOR_AMPS_PER_VOLT,25,400,624,972,5,MAX_BATTERY_AMPS,MAX_BATTERY_AMPS_REGEN,MAX_MOTOR_AMPS,MAX_MOTOR_AMPS,DEFAULT_PRECHARGE_TIME,{0},0
};

const SavedValuesStruct2 savedValuesDefault2 = {
	DEFAULT_ANGLE_OFFSET, DEFAULT_ROTOR_TIME_CONSTANT_INDEX,DEFAULT_NUM_POLE_PAIRS,DEFAULT_MAX_MECHANICAL_RPM,DEFAULT_THROTTLE_TYPE, DEFAULT_ENCODER_TICKS,DEFAULT_DATA_TO_DISPLAY_SET1, DEFAULT_DATA_TO_DISPLAY_SET2,0,{0,0,0,0,0,0}, 0
};
volatile SavedValuesStruct2 savedValues2 = {
	DEFAULT_ANGLE_OFFSET, DEFAULT_ROTOR_TIME_CONSTANT_INDEX,DEFAULT_NUM_POLE_PAIRS,DEFAULT_MAX_MECHANICAL_RPM,DEFAULT_THROTTLE_TYPE, DEFAULT_ENCODER_TICKS,DEFAULT_DATA_TO_DISPLAY_SET1, DEFAULT_DATA_TO_DISPLAY_SET2,0,{0,0,0,0,0,0}, 0
};

#endif

unsigned int revCounterMax = 160000L/(4*DEFAULT_ENCODER_TICKS);  // revCounterMax = 160000L / (4*savedValues2.encoderTicks);  // 4* because I'm doing 4 times resolution for the encoder. 16,000,000 because revolutions per 16 seconds is computed as:  16*10,000*poscnt * rev/(maxPosCnt*revcounter*(16sec))

// This is always a copy of the data that's in the EE PROM.
// To change EE Prom, modify this, and then copy it to EE Prom.

int EEDataInRam1[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int EEDataInRam2[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int EEDataInRam3[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int EEDataInRam4[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

_prog_addressT EE_addr1 = 0x7ffc00;
_prog_addressT EE_addr2 = 0x7ffc20;
_prog_addressT EE_addr3 = 0x7ffc40;
_prog_addressT EE_addr4 = 0x7ffc60;

// celciusToResistance[0] is the resistance in Ohms that corresponds to 0 degrees celcius.  celciusToResistance[59] is the resistance in Ohms that corresponds to 59 degrees celcius.
// the range is 0 celcius to 126 degrees celcius.
//									0, 		1,	2,		3,	4,		5,	6,		7,	8,		9,	10,		11,	12,		13,	14,		15,	16,		17,	18,		19,	20,		21,	22,		23,	24,		25,	26,	27,	  28,	29, 30,  31,   32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,
const int celciusToResistance[] = {32624,31175,29727,28278,26830,25381,24284,23187,22091,20994,19897,19060,18223,17385,16548,15711,15067,14424,13780,13137,12493,11994,11496,10997,10499,10000,9611,9222,8834,8445,8056,7751,7445,7140,6835,6530,6289,6047,5806,5565,5324,5132,4940,4749,4557,4365,4212,4059,3905,3752,3599,3475,3352,3229,3106,2982,2883,2783,2683,2584,2484,2403,2322,2241,2160,2079,2013,1946,1880,1814,1748,1693,1639,1585,1530,1476,1431,1386,1341,1297,1252,1215,1178,1140,1103,1066,1035,1004,973,942,912,886,860,834,808,782,761,739,717,696,674,656,638,619,601,583,567,552,537,521,506,493,479,466,453,440,429,418,407,396,384,375,365,356,346,337,327,
};

// = loopPeriod / rotorTimeConstant * 2^18.  loopPeriod is 0.0001 seconds, because it's being run at 10KHz. Rotor time constants range from 0.005 to 0.150 seconds.
// After using an element from this array, you must eventually divide the result by 2^18!!!
// rotorTimeConstantArray1[0] corresponds to a rotorTimeConstant of 0.005 seconds.  rotorTimeConstantArray1[145] <=> rotor time constant of 0.150 sec.
const unsigned int rotorTimeConstantArray1[] = {
//    0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31   32   33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48   49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64   65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80   81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96   97   98   99  100  101  102  103  104  105  106  107  108  109  110  111  112  113  114  115  116  117  118  119  120  121  122  123  124  125  126  127  128  129  130  131  132  133  134  135  136  137  138  139  140  141  142  143  144  145 
// .005,.006,.007,.008,.009,.010,.011,.012,.013,.014,.015,.016,.017,.018,.019,.020,.021,.022,.023,.024,.025,.026,.027,.028,.029,.030,.031,.032,.033,.034,.035,.036,.037,.038,.039,.040,.041,.042,.043,.044,.045,.046,.047,.048,.049,.050,.051,.052,.053,.054,.055,.056,.057,.058,.059,.060,.061,.062,.063,.064,.065,.066,.067,.068,.069,.070,.071,.072,.073,.074,.075,.076,.077,.078,.079,.080,.081,.082,.083,.084,.085,.086,.087,.088,.089,.090,.091,.092,.093,.094,.095,.096,.097,.098,.099,.100,.101,.102,.103,.104,.105,.106,.107,.108,.109,.110,.111,.112,.113,.114,.115,.116,.117,.118,.119,.120,.121,.122,.123,.124,.125,.126,.127,.128,.129,.130,.131,.132,.133,.134,.135,.136,.137,.138,.139,.140,.141,.142,.143,.144,.145,.146,.147,.148,.149,.150
   5243,4369,3745,3277,2913,2621,2383,2185,2016,1872,1748,1638,1542,1456,1380,1311,1248,1192,1140,1092,1049,1008, 971, 936, 904, 874, 846, 819, 794, 771, 749, 728, 708, 690, 672, 655, 639, 624, 610, 596, 583, 570, 558, 546, 535, 524, 514, 504, 495, 485, 477, 468, 460, 452, 444, 437, 430, 423, 416, 410, 403, 397, 391, 386, 380, 374, 369, 364, 359, 354, 350, 345, 340, 336, 332, 328, 324, 320, 316, 312, 308, 305, 301, 298, 295, 291, 288, 285, 282, 279, 276, 273, 270, 267, 265, 262, 260, 257, 255, 252, 250, 247, 245, 243, 240, 238, 236, 234, 232, 230, 228, 226, 224, 222, 220, 218, 217, 215, 213, 211, 210, 208, 206, 205, 203, 202, 200, 199, 197, 196, 194, 193, 191, 190, 189, 187, 186, 185, 183, 182, 181, 180, 178, 177, 176, 175
};
// (1/rotorTimeConstant) * 1/(2*pi) * 2^11.  I'm trying to keep all of them in integer range.
// rotorTimeConstantArray2[0] corresponds to a rotorTimeConstant of 0.005 seconds.  rotorTimeConstantArray2[145] = 0.150.
const unsigned int rotorTimeConstantArray2[] = {
//    0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21    22    23    24    25    26    27   28   29   30   31   32   33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48   49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64   65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80   81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96   97   98   99  100  101  102  103  104  105  106  107  108  109  110  111  112  113  114  115  116  117  118  119  120  121  122  123  124  125  126  127  128  129  130  131  132  133  134  135  136  137  138  139  140  141  142  143  144  145 
// .005, .006, .007, .008, .009, .010, .011, .012, .013, .014, .015, .016, .017, .018, .019, .020, .021, .022, .023, .024, .025, .026, .027, .028, .029, .030, .031, .032,.033,.034,.035,.036,.037,.038,.039,.040,.041,.042,.043,.044,.045,.046,.047,.048,.049,.050,.051,.052,.053,.054,.055,.056,.057,.058,.059,.060,.061,.062,.063,.064,.065,.066,.067,.068,.069,.070,.071,.072,.073,.074,.075,.076,.077,.078,.079,.080,.081,.082,.083,.084,.085,.086,.087,.088,.089,.090,.091,.092,.093,.094,.095,.096,.097,.098,.099,.100,.101,.102,.103,.104,.105,.106,.107,.108,.109,.110,.111,.112,.113,.114,.115,.116,.117,.118,.119,.120,.121,.122,.123,.124,.125,.126,.127,.128,.129,.130,.131,.132,.133,.134,.135,.136,.137,.138,.139,.140,.141,.142,.143,.144,.145,.146,.147,.148,.149,.150
  65190,54325,46564,40744,36217,32595,29632,27162,25073,23282,21730,20372,19173,18108,17155,16297,15521,14816,14172,13581,13038,12537,12072,11641,11240,10865,10514,10186,9877,9587,9313,9054,8809,8578,8358,8149,7950,7761,7580,7408,7243,7086,6935,6791,6652,6519,6391,6268,6150,6036,5926,5821,5718,5620,5525,5432,5343,5257,5174,5093,5015,4939,4865,4793,4724,4656,4591,4527,4465,4405,4346,4289,4233,4179,4126,4074,4024,3975,3927,3880,3835,3790,3747,3704,3662,3622,3582,3543,3505,3468,3431,3395,3360,3326,3292,3259,3227,3196,3165,3134,3104,3075,3046,3018,2990,2963,2936,2910,2885,2859,2834,2810,2786,2762,2739,2716,2694,2672,2650,2629,2608,2587,2567,2546,2527,2507,2488,2469,2451,2432,2414,2397,2379,2362,2345,2328,2312,2295,2279,2264,2248,2233,2217,2202,2188,2173
};

// This one is hard to explain.  It's to quickly find distance from (Vd, Vq) to (0,0).  (Vd,Vq) must be clamped to some maximum radius.  The fast distance I use isn't very accurate, so I add this correction.
// Let's say Vd <= Vq.  You find the index for this array by doing index = 1024 * Vd / Vq.  This works because the %error from fastDistance(Vd,Vq) is the same as long as Vd/Vq is the same.
// In other words, if Vd1/Vq1 = Vd2/Vq2, then fastDistance(Vd1,Vq1)/RealDistance(Vd1,Vq1) = fastDistance(Vd2,Vq2)/RealDistance(Vd2,Vq2).
// All this just to avoid doing a square root.  haha.
// It has been scaled up by 2^15.  So, you must shift down by 15 after multiplying by distCorrection[].
// array size is 1025.  distCorrection[0] up to distCorrection[1024] inclusive.
const unsigned int distCorrection[] = {
32768,32758,32748,32738,32728,32718,32709,32699,32689,32680,32670,32660,32651,32641,32632,32622,32613,32603,32594,32585,32575,32566,32557,32548,32539,32530,32521,32512,32503,32494,32485,32476,32467,32458,32449,32441,32432,32423,32415,32406,32398,32389,32381,32372,32364,32355,32347,32339,32330,32322,32314,32306,32298,32290,32282,32274,32266,32258,32250,32242,32234,32226,32218,32211,32203,32195,32188,32180,32173,32165,32158,32150,32143,32135,32128,32121,32113,32106,32099,32092,32085,32077,32070,32063,32056,32049,32042,32036,32029,32022,32015,32008,32002,31995,31988,31982,31975,31968,31962,31955,31949,31942,31936,31930,31923,
31917,31911,31905,31898,31892,31886,31880,31874,31868,31862,31856,31850,31844,31838,31832,31827,31821,31815,31810,31804,31798,31793,31787,31782,31776,31771,31765,31760,31754,31749,31744,31738,31733,31728,31723,31718,31713,31708,31702,31697,31692,31688,31683,31678,31673,31668,31663,31658,31654,31649,31644,31640,31635,31631,31626,31622,31617,31613,31608,31604,31600,31595,31591,31587,31582,31578,31574,31570,31566,31562,31558,31554,31550,31546,31542,31538,31534,31530,31526,31523,31519,31515,31512,31508,31504,31501,31497,31494,31490,31487,31483,31480,31477,31473,31470,31467,31463,31460,31457,31454,31451,31448,31444,31441,31438,
31435,31432,31429,31427,31424,31421,31418,31415,31413,31410,31407,31404,31402,31399,31397,31394,31392,31389,31387,31384,31382,31379,31377,31375,31372,31370,31368,31366,31363,31361,31359,31357,31355,31353,31351,31349,31347,31345,31343,31341,31339,31338,31336,31334,31332,31331,31329,31327,31326,31324,31322,31321,31319,31318,31316,31315,31314,31312,31311,31310,31308,31307,31306,31304,31303,31302,31301,31300,31299,31298,31297,31296,31295,31294,31293,31292,31291,31290,31289,31289,31288,31287,31286,31286,31285,31284,31284,31283,31282,31282,31281,31281,31280,31280,31280,31279,31279,31279,31278,31278,31278,31277,31277,31277,31277,
31277,31277,31277,31276,31276,31276,31276,31276,31277,31277,31277,31277,31277,31277,31277,31278,31278,31278,31278,31279,31279,31280,31280,31280,31281,31281,31282,31282,31283,31283,31284,31285,31285,31286,31287,31287,31288,31289,31290,31290,31291,31292,31293,31294,31295,31296,31297,31298,31299,31300,31301,31302,31303,31304,31305,31306,31308,31309,31310,31311,31313,31314,31315,31317,31318,31319,31321,31322,31324,31325,31327,31328,31330,31331,31333,31335,31336,31338,31340,31341,31343,31345,31347,31348,31350,31352,31354,31356,31358,31360,31362,31364,31366,31368,31370,31372,31374,31376,31378,31380,31382,31384,31387,31389,31391,
31393,31396,31398,31400,31403,31405,31407,31410,31412,31415,31417,31420,31422,31425,31427,31430,31432,31435,31438,31440,31443,31446,31448,31451,31454,31457,31459,31462,31465,31468,31471,31474,31476,31479,31482,31485,31488,31491,31494,31497,31500,31503,31507,31510,31513,31516,31519,31522,31526,31529,31532,31535,31539,31542,31545,31549,31552,31555,31559,31562,31566,31569,31572,31576,31579,31583,31587,31590,31594,31597,31601,31605,31608,31612,31616,31619,31623,31627,31630,31634,31638,31642,31646,31650,31653,31657,31661,31665,31669,31673,31677,31681,31685,31689,31693,31697,31701,31705,31709,31713,31718,31722,31726,31730,31734,
31739,31743,31747,31751,31756,31760,31764,31769,31773,31777,31782,31786,31791,31795,31800,31804,31808,31813,31817,31822,31827,31831,31836,31840,31845,31850,31854,31859,31864,31868,31873,31878,31882,31887,31892,31897,31902,31906,31911,31916,31921,31926,31931,31936,31941,31946,31951,31956,31961,31966,31971,31976,31981,31986,31991,31996,32001,32006,32011,32016,32022,32027,32032,32037,32042,32048,32053,32058,32063,32069,32074,32079,32085,32090,32095,32101,32106,32112,32117,32123,32128,32133,32139,32144,32150,32155,32161,32167,32172,32178,32183,32189,32195,32200,32206,32212,32217,32223,32229,32234,32240,32246,32252,32257,32263,
32269,32275,32281,32286,32292,32298,32304,32310,32316,32322,32328,32334,32339,32345,32351,32357,32363,32369,32375,32382,32388,32394,32400,32406,32412,32418,32424,32430,32436,32443,32449,32455,32461,32467,32474,32480,32486,32492,32499,32505,32511,32518,32524,32530,32537,32543,32549,32556,32562,32569,32575,32581,32588,32594,32601,32607,32614,32620,32627,32633,32640,32646,32653,32660,32666,32673,32679,32686,32693,32699,32706,32713,32719,32726,32733,32739,32746,32753,32759,32766,32773,32780,32787,32793,32800,32807,32814,32821,32827,32834,32841,32848,32855,32862,32869,32876,32883,32890,32896,32903,32910,32917,32924,32931,32938,
32945,32952,32960,32967,32974,32981,32988,32995,33002,33009,33016,33023,33031,33038,33045,33052,33059,33066,33074,33081,33088,33095,33102,33110,33117,33124,33132,33139,33146,33153,33161,33168,33175,33183,33190,33198,33205,33212,33220,33227,33234,33242,33249,33257,33264,33272,33279,33287,33294,33302,33309,33317,33324,33332,33339,33347,33354,33362,33369,33377,33385,33392,33400,33407,33415,33423,33430,33438,33446,33453,33461,33469,33476,33484,33492,33499,33507,33515,33523,33530,33538,33546,33554,33561,33569,33577,33585,33593,33600,33608,33616,33624,33632,33640,33648,33655,33663,33671,33679,33687,33695,33703,33711,33719,33727,
33735,33743,33751,33759,33767,33775,33783,33791,33799,33807,33815,33823,33831,33839,33847,33855,33863,33871,33879,33887,33895,33903,33912,33920,33928,33936,33944,33952,33960,33969,33977,33985,33993,34001,34010,34018,34026,34034,34042,34051,34059,34067,34075,34084,34092,34100,34109,34117,34125,34133,34142,34150,34158,34167,34175,34183,34192,34200,34208,34217,34225,34234,34242,34250,34259,34267,34276,34284,34292,34301,34309,34318,34326,34335,34343,34352,34360,34369,34377,34386,34394,34403,34411,34420,34428,34437,34445,34454,34462,34471,34479,34488,34497,34505,34514,34522,34531,34539,34548,34557,34565,34574,34583,34591,34600,
34608,34617,34626,34634,34643,34652,34660,34669,34678,34687,34695,34704,34713,34721,34730,34739,34748,34756,34765,34774,34782,34791,34800,34809,34818,34826,34835,34844,34853,34861,34870,34879,34888,34897,34906,34914,34923,34932,34941,34950,34959,34967,34976,34985,34994,35003,35012,35021,35030,35038,35047,35056,35065,35074,35083,35092,35101,35110,35119,35128,35137,35146,35154,35163,35172,35181,35190,35199,35208,35217,35226,35235,35244,35253,35262,35271,35280,35289,35298,35307,35316,35325,35334,35343,
};

// Ex:  If you have a right triangle, given hypotenuse C and leg A, this gives you the unknown leg B.  C is normalized to 1024.  Leg A is in [0, 1023].  The unknown Leg B is also in [0, 1023].  Leg B = sqrt(C^2 - A^2).
// This is used in the correction for an interior permanent magnet motor.  Throttle command ("current radius") is the hypotenuse.  Given a normalized "current radius" in [0, 1023], let's see what <IdRefRef,IqRefRef> gives the best acceleration.  That would be best torque I guess for a given currentRadius = sqrt(Id^2 + Iq^2).
// That would let you solve for K:
// From that TI video (teaching old motors new tricks part 4), after simplifying some, you get Id = -K + sqrt(K*K - currentRadius*currentRadius/2).  This is of the form of the quadratic formula.  x = (-b +/- sqrt(b^2 -4ac))/(2a), where ax*x + bx + c = 0.  So, from that, comparing terms, we find that b = K, c = currentRadius*currentRadius/4, a = 1/2, and x = Id.  So, we have
// 1/2*Id*Id + K*Id + currentRadius*currentRadius/4 = 0		// ax*x + bx + c = 0
// Now, solve for K:
// K = (currentRadius*currentRadius + 2*Id*Id)/(-4*Id).  Write it that way so as to not lose resolution by dividing too early.  We want a big numerator before dividing.
// I think K = 0.25*lambdaMagnets/(Ld - Lq).  I don't have a friggen clue what the values Ld, Lq, or lambdaMagnets are.  But who cares?  Then, you can use the following formulas to compute IdRef and IqRef, given the "current radius":
// IdRefRef = -K + sqrt(K*K - (currentRadius*currentRadius/sqrt(2))^2). **ALWAYS NEGATIVE**
// IqRefRef = sign()*sqrt(currentRadius^2 - IdRefRef^2).  It is negative if you are wanting to do regen, and is positive otherwise.
// Then, if <Vd,Vq> has too large of a radius to successfully command the feedback currents to IqRefRef and IdRefRef, you scale down Vd & Vq & IqRef & IdRef so that radius(Vd,Vq) is once again safely inside the available voltage disk.  This is the "field weakening" of sorts.
// Then, you let IqRef and IdRef ramp back to IqRefRef and IdRefRef.  Like teenagers.  You knock them back away from the boundary, and they start creeping up again.
// 
// Entries 0 thorugh 1023 inclusive.
const int unknownTriangleLegLookup[] = {
1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1022,1022,1022,1022,1022,1022,1022,1022,1022,1022,1022,1022,1022,1022,1022,1022,1021,1021,1021,1021,1021,1021,1021,1021,1021,1021,1021,1021,1021,1020,1020,1020,1020,1020,1020,1020,1020,1020,1020,1020,1019,1019,1019,1019,1019,1019,1019,1019,1019,1019,1018,1018,1018,1018,1018,1018,1018,1018,1018,1018,1017,1017,1017,1017,1017,1017,1017,1017,1016,1016,1016,1016,1016,1016,1016,1016,1015,1015,1015,1015,1015,1015,1015,1015,1014,1014,1014,1014,1014,1014,1014,1013,1013,
1013,1013,1013,1013,1013,1012,1012,1012,1012,1012,1012,1011,1011,1011,1011,1011,1011,1010,1010,1010,1010,1010,1010,1009,1009,1009,1009,1009,1009,1008,1008,1008,1008,1008,1008,1007,1007,1007,1007,1007,1006,1006,1006,1006,1006,1005,1005,1005,1005,1005,1004,1004,1004,1004,1004,1003,1003,1003,1003,1003,1002,1002,1002,1002,1002,1001,1001,1001,1001,1001,1000,1000,1000,1000,999,999,999,999,999,998,998,998,998,997,997,997,997,996,996,996,996,995,995,995,995,995,994,994,994,994,993,993,993,993,992,992,992,991,991,991,991,990,990,990,990,989,989,989,989,988,988,988,987,987,987,987,986,986,986,986,985,985,985,984,984,984,984,983,983,983,982,982,982,981,981,981,981,980,980,980,979,979,979,978,978,978,978,977,977,977,976,976,976,975,975,975,974,974,974,973,
973,973,972,972,972,971,971,971,970,970,970,969,969,969,968,968,968,967,967,967,966,966,966,965,965,964,964,964,963,963,963,962,962,962,961,961,960,960,960,959,959,959,958,958,958,957,957,956,956,956,955,955,954,954,954,953,953,952,952,952,951,951,950,950,950,949,949,948,948,948,947,947,946,946,946,945,945,944,944,943,943,943,942,942,941,941,941,940,940,939,939,938,938,937,937,937,936,936,935,935,934,934,933,933,933,932,932,931,931,930,930,929,929,928,928,927,927,927,926,926,925,925,924,924,923,923,922,922,921,921,920,920,919,919,918,918,917,917,916,916,915,915,914,914,913,913,912,912,911,911,910,910,909,909,908,908,907,907,906,906,905,905,904,903,903,902,902,901,901,900,900,899,899,898,898,897,896,896,895,895,894,894,893,893,892,891,891,890,890,889,889,
888,887,887,886,886,885,884,884,883,883,882,882,881,880,880,879,879,878,877,877,876,876,875,874,874,873,873,872,871,871,870,869,869,868,868,867,866,866,865,864,864,863,862,862,861,861,860,859,859,858,857,857,856,855,855,854,853,853,852,851,851,850,849,849,848,847,847,846,845,845,844,843,843,842,841,840,840,839,838,838,837,836,836,835,834,833,833,832,831,831,830,829,828,828,827,826,825,825,824,823,822,822,821,820,819,819,818,817,816,816,815,814,813,813,812,811,810,810,809,808,807,806,806,805,804,803,803,802,801,800,799,799,798,797,796,795,795,794,793,792,791,790,790,789,788,787,786,785,785,784,783,782,781,780,780,779,778,777,776,775,774,774,773,772,771,770,769,768,767,767,766,765,764,763,762,761,760,759,758,758,757,756,755,754,753,752,751,750,749,748,747,
746,746,745,744,743,742,741,740,739,738,737,736,735,734,733,732,731,730,729,728,727,726,725,724,723,722,721,720,719,718,717,716,715,714,713,712,711,710,709,708,707,706,705,704,703,701,700,699,698,697,696,695,694,693,692,691,690,688,687,686,685,684,683,682,681,680,678,677,676,675,674,673,672,670,669,668,667,666,665,663,662,661,660,659,658,656,655,654,653,652,650,649,648,647,645,644,643,642,640,639,638,637,635,634,633,632,630,629,628,626,625,624,623,621,620,619,617,616,615,613,612,611,609,608,607,605,604,602,601,600,598,597,596,594,593,591,590,588,587,586,584,583,581,580,578,577,575,574,573,571,570,568,567,565,564,562,560,559,557,556,554,553,551,550,548,546,545,543,542,540,538,537,535,534,532,530,529,527,525,524,522,520,519,517,515,513,512,510,508,506,505,
503,501,499,498,496,494,492,490,488,487,485,483,481,479,477,475,473,471,470,468,466,464,462,460,458,456,454,452,450,448,446,443,441,439,437,435,433,431,429,426,424,422,420,418,415,413,411,408,406,404,402,399,397,394,392,390,387,385,382,380,377,375,372,370,367,364,362,359,356,354,351,348,345,343,340,337,334,331,328,325,322,319,316,313,310,307,303,300,297,294,290,287,283,280,276,273,269,265,262,258,254,250,246,242,238,234,229,225,220,216,211,206,201,196,191,186,180,175,169,163,156,150,143,135,128,120,111,101,90,78,64,45,
};

// How about this?  For each saliencyConstant, time how long it takes to "run out of voltage", starting at zero RPM  It has no problem commanding a current disk of any radius near zero RPM.
// these are the K values, for Id = 0 down to -512, and currentRadius = 512.  It's a measure of the "saliency" of the motor poles.  Are the magnets on the surface, or are they buried?  If K is large in magnitude, the poles are right at the surface, and you have a standard permanent magnet motor.  If the magnitude of K is small, the magnets are buried deeper, and you have an internal permanent magnet motor, and I guess the motor is more like a synchronous reluctance motor.
// 
// entry 0 is the K that corresponds to Id = 0, currentRadius = 512.
// entry 512 is the K that corresponds to Id = -512, currentRadius = 512.
const int saliencyConstant[] = {
32767,32767,32767,21847,16386,13110,10926,9366,8196,7286,6559,5963,5467,5048,4688,4377,4104,3864,3650,3459,3287,3131,2990,2861,2743,2634,2534,2441,2355,2274,2200,2130,2064,2002,1945,1890,1838,1790,1744,1700,1658,1619,1581,1546,1511,1479,1448,1418,1389,1362,1336,1311,1286,1263,1241,1219,1198,1178,1159,1140,1122,1105,1088,1072,1056,1041,1026,1012,998,984,971,959,946,934,923,911,900,890,879,869,859,850,840,831,822,814,805,797,789,781,773,766,758,751,744,737,731,724,718,711,705,699,694,688,682,677,671,666,661,656,651,646,641,636,632,627,623,619,614,610,606,602,598,594,591,587,583,580,576,573,569,566,562,559,556,553,550,547,544,541,538,535,533,530,527,524,522,519,517,514,512,510,507,505,503,500,498,496,494,492,490,488,486,484,482,480,478,476,474,472,
471,469,467,465,464,462,460,459,457,456,454,453,451,450,448,447,445,444,443,441,440,439,437,436,435,434,432,431,430,429,428,427,425,424,423,422,421,420,419,418,417,416,415,414,413,412,411,411,410,409,408,407,406,405,405,404,403,402,401,401,400,399,398,398,397,396,396,395,394,394,393,392,392,391,391,390,389,389,388,388,387,387,386,386,385,385,384,384,383,383,382,382,381,381,380,380,379,379,379,378,378,377,377,377,376,376,375,375,375,374,374,374,373,373,373,372,372,372,372,371,371,371,370,370,370,370,369,369,369,369,368,368,368,368,368,367,367,367,367,367,366,366,366,366,366,366,365,365,365,365,365,365,365,364,364,364,364,364,364,364,364,363,363,363,363,363,363,363,363,363,363,363,363,363,363,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,
362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,364,364,364,364,364,364,364,364,364,364,365,365,365,365,365,365,365,365,365,366,366,366,366,366,366,366,366,367,367,367,367,367,367,367,368,368,368,368,368,368,368,369,369,369,369,369,369,370,370,370,370,370,370,371,371,371,371,371,372,372,372,372,372,372,373,373,373,373,373,374,374,374,374,374,375,375,375,375,375,376,376,376,376,377,377,377,377,377,378,378,378,378,379,379,379,379,379,380,380,380,380,381,381,381,381,382,382,382,382,383,383,383,383,384,384,384,
};

// 512 possible values for sin.  for x in [-1, 1] = [-32767, 32767].  So the scale is about 2^15 - 1.  Let's pretend it's 2^15.  haha.
const int _sin_times32768[] =  
{0, 	402,	804,	1206,	1608,	2009,	2410,	2811,	3212,	3612,	4011,	4410,	4808,	5205,	5602,	5998,	6393,	6786,	7179,	7571,	7962,
8351,	8739,	9126,	9512,	9896,	10278,	10659,	11039,	11417,	11793,	12167,	12539,	12910,	13279,	13645,	14010,	14372,	14732,	15090,	15446,
15800,	16151,	16499,	16846,	17189,	17530,	17869,	18204,	18537,	18868,	19195,	19519,	19841,	20159,	20475,	20787,	21096,	21403,	21705,	22005,
22301,	22594,	22884,	23170,	23452,	23731,	24007,	24279,	24547,	24811,	25072,	25329,	25582,	25832,	26077,	26319,	26556,	26790,	27019,	27245,
27466,	27683,	27896,	28105,	28310,	28510,	28706,	28898,	29085,	29268,	29447,	29621,	29791,	29956,	30117,	30273,	30424,	30571,	30714,	30852,
30985,	31113,	31237,	31356,	31470,	31580,	31685,	31785,	31880,	31971,	32057,	32137,	32213,	32285,	32351,	32412,	32469,	32521,	32567,	32609,
32646,	32678,	32705,	32728,	32745,	32757,	32765,	32767,	32765,	32757,	32745,	32728,	32705,	32678,	32646,	32609,	32567,	32521,	32469,	32412,
32351,	32285,	32213,	32137,	32057,	31971,	31880,	31785,	31685,	31580,	31470,	31356,	31237,	31113,	30985,	30852,	30714,	30571,	30424,	30273,
30117,	29956,	29791,	29621,	29447,	29268,	29085,	28898,	28706,	28510,	28310,	28105,	27896,	27683,	27466,	27245,	27019,	26790,	26556,	26319,
26077,	25832,	25582,	25329,	25072,	24811,	24547,	24279,	24007,	23731,	23452,	23170,	22884,	22594,	22301,	22005,	21705,	21403,	21096,	20787,
20475,	20159,	19841,	19519,	19195,	18868,	18537,	18204,	17869,	17530,	17189,	16846,	16499,	16151,	15800,	15446,	15090,	14732,	14372,	14010,
13645,	13279,	12910,	12539,	12167,	11793,	11417,	11039,	10659,	10278,	9896,	9512,	9126,	8739,	8351,	7962,	7571,	7179,	6786,	6393,
5998,	5602,	5205,	4808,	4410,	4011,	3612,	3212,	2811,	2410,	2009,	1608,	1206,	804,	402,	0,  	-402,	-804,	-1206,	-1608,
-2009,	-2410,	-2811,	-3212,	-3612,	-4011,	-4410,	-4808,	-5205,	-5602,	-5998,	-6393,	-6786,	-7179,	-7571,	-7962,	-8351,	-8739,	-9126,	-9512,
-9896,	-10278,	-10659,	-11039,	-11417,	-11793,	-12167,	-12539,	-12910,	-13279,	-13645,	-14010,	-14372,	-14732,	-15090,	-15446,	-15800,	-16151,	-16499,	-16846,
-17189,	-17530,	-17869,	-18204,	-18537,	-18868,	-19195,	-19519,	-19841,	-20159,	-20475,	-20787,	-21096,	-21403,	-21705,	-22005,	-22301,	-22594,	-22884,	-23170,
-23452,	-23731,	-24007,	-24279,	-24547,	-24811,	-25072,	-25329,	-25582,	-25832,	-26077,	-26319,	-26556,	-26790,	-27019,	-27245,	-27466,	-27683,	-27896,	-28105,
-28310,	-28510,	-28706,	-28898,	-29085,	-29268,	-29447,	-29621,	-29791,	-29956,	-30117,	-30273,	-30424,	-30571,	-30714,	-30852,	-30985,	-31113,	-31237,	-31356,
-31470,	-31580,	-31685,	-31785,	-31880,	-31971,	-32057,	-32137,	-32213,	-32285,	-32351,	-32412,	-32469,	-32521,	-32567,	-32609,	-32646,	-32678,	-32705,	-32728,
-32745,	-32757,	-32765,	-32767,	-32765,	-32757,	-32745,	-32728,	-32705,	-32678,	-32646,	-32609,	-32567,	-32521,	-32469,	-32412,	-32351,	-32285,	-32213,	-32137,
-32057,	-31971,	-31880,	-31785,	-31685,	-31580,	-31470,	-31356,	-31237,	-31113,	-30985,	-30852,	-30714,	-30571,	-30424,	-30273,	-30117,	-29956,	-29791,	-29621,
-29447,	-29268,	-29085,	-28898,	-28706,	-28510,	-28310,	-28105,	-27896,	-27683,	-27466,	-27245,	-27019,	-26790,	-26556,	-26319,	-26077,	-25832,	-25582,	-25329,
-25072,	-24811,	-24547,	-24279,	-24007,	-23731,	-23452,	-23170,	-22884,	-22594,	-22301,	-22005,	-21705,	-21403,	-21096,	-20787,	-20475,	-20159,	-19841,	-19519,
-19195,	-18868,	-18537,	-18204,	-17869,	-17530,	-17189,	-16846,	-16499,	-16151,	-15800,	-15446,	-15090,	-14732,	-14372,	-14010,	-13645,	-13279,	-12910,	-12539,
-12167,	-11793,	-11417,	-11039,	-10659,	-10278,	-9896,	-9512,	-9126,	-8739,	-8351,	-7962,	-7571,	-7179,	-6786,	-6393,	-5998,	-5602,	-5205,	-4808,
-4410,	-4011,	-3612,	-3212,	-2811,	-2410,	-2009,	-1608,	-1206,	-804,	-402, 		0,		0,		0,		0,		0,		0,		0,		0,};
////////////////////////////////////////////////////////////////

volatile int bigArray[514];
volatile rotorTestType myRotorTest = {0,0,0,DEFAULT_ROTOR_TIME_CONSTANT_INDEX,0,0};
volatile angleOffsetTestType myAngleOffsetTest     = {0,0,0,0,0,0,0};
volatile motorSaliencyTestType myMotorSaliencyTest = {0,0,0,0,0,0,0};
volatile firstEncoderIndexPulseTestType myFirstEncoderIndexPulseTest = {0,0,1};		// this starts with the test running, since at startup, you don't know the absolute position of the motor post until an index pulse has happened.

volatile piType myPI;

volatile int poscnt = 0;
volatile int poscntOld = 0;
volatile int rotorFluxRPS_times16 = 0;

volatile unsigned int rotorFluxAngle = 1; 		// This is the rotor flux angle. In [0, 511]
volatile int RPS_times16 = 0; // range [-3200, 3200], where 3200 corresponds to 200rev/sec = 12,000RPM, and 0 means 0RPM.
volatile int currentSensorAmpsPerVoltTimes5 = DEFAULT_CURRENT_SENSOR_AMPS_PER_VOLT*5;


volatile int maxRPS_times16 = DEFAULT_MAX_RPS_TIMES16; // 3200 CORRESPONDS TO 12000RPM.

volatile unsigned int rGlobal = 0;
volatile long rGlobal_filtered_times65536 = 0L;//   
volatile int rGlobal_filtered = 0;//
volatile int voltageDiskExceeded = 0;

volatile int slipSpeedRPS_times16 = 0;
volatile int magnetizingCurrentAmps_times8 = 0;

volatile unsigned int counter1k = 0;
volatile unsigned int counter10k = 0;

volatile unsigned int faultBits = STARTUP_FAULT;
volatile int vRef1 = 512, vRef2 = 512;  // these are temporary values for "zero current feedback".  The real values will be computed later, but this is close, since zero current corresponds to 2.5v on the current sensor.

volatile long throttleSum = 0;
volatile int throttle = 0, rawThrottle = 0;
volatile int throttleFaultCounter = 0;

volatile int maxMotorCurrentNormalizedRegen = 0;
volatile int maxMotorCurrentNormalized = 0;
volatile int batteryCurrentNormalized = 0;
volatile int batteryAmps = 0;
volatile int maxBatteryCurrentNormalized = 0;
volatile int maxBatteryCurrentNormalizedRegen = 0;
volatile int normalizedToAmpsMultiplier = 0;
volatile long batteryCurrentSum = 0;

volatile int temperatureMultiplier = 8;
volatile int temperatureBasePlate = 0;

volatile int ADCurrent1 = 0, ADCurrent2 = 0;

// _ADInterrupt() variables
volatile int i_alpha = 0;
volatile int i_beta = 0;
volatile int Id = 0;
volatile int Iq = 0;
volatile int Ia = 0, Ib = 0, Ic = 0, Ib_times2 = 0;
volatile int Vd = 0, Vq = 0;
volatile int v_alpha = 0;
volatile int v_beta = 0;
volatile int pdc1 = 0;
volatile int pdc2 = 0;
volatile int pdc3 = 0;
volatile int averageDuty = 0;
volatile int Va = 0, Vb = 0, Vc = 0;
volatile int IdRef = 0;	// in the range [0, 4096]
volatile int IdRefRef = 0;
volatile int IqRefRef = 0;
volatile int IqRef = 0; // in the range [-4096, 4096]
volatile int currentRadiusRefRef = 0;
volatile int currentRadiusRefRefOverSqrt2 = 0;
volatile int K = 0;
volatile long int KLong = 0L;

extern char intString[];

extern void u16_to_str(char *str, unsigned val, unsigned char digits);
extern int TransmitString(char* str);
extern void ShowMenu(void);
extern void ProcessCommand(void);
extern void InitUART2(void);
extern void StreamData(void);
extern volatile dataStream myDataStream;

void InitTimers();
void InitIORegisters(void);
void Delay(unsigned int);
void DelayTenthsSecond(int time);
void DelaySeconds(int time);
void InitADAndPWM();
void InitQEI();
void TurnOffADAndPWM();
void InitCNModule();
void InitPIStruct();
void ClearAllFaults();  // clear the flip flop fault and the desat fault.
void ClearDesatFault();
void ClearFlipFlop();
void ComputeRotorFluxAngle();
void ComputeRotorFluxAngleSensorless();
void SpaceVectorModulation();
void ClampVdVq();
void Delay1uS();
void GrabDataSnapshot();

void __attribute__ ((__interrupt__,auto_psv)) _CNInterrupt(void);
void __attribute__ ((__interrupt__,auto_psv)) _ADCInterrupt(void);
void __attribute__ ((__interrupt__,auto_psv)) _MathError(void);
void __attribute__ ((__interrupt__,auto_psv)) _StackError(void);
void __attribute__ ((__interrupt__,auto_psv)) _AddressError(void);
void __attribute__ ((__interrupt__,auto_psv)) _OscillatorFail(void);

void MoveDataFromEEPromToRAM();
void EESaveValues();
void InitializeThrottleAndCurrentVariables();
void MoveToNextPIValues();
int unknownTriangleLeg(int c, int b);


int main() {
	int i = 0;
	unsigned int now = 0;
	long int vRef1Sum = 0;
	long int vRef2Sum = 0;
	int localCurrent1 = 0;
	int localCurrent2 = 0;
	unsigned int notShownFaultYet = 0x0FFFF;	


	InitIORegisters();
	InitTimers();  // Now timer1 is running at around 59KHz.
	DelayTenthsSecond(5); 
	// Let voltages settle.
	O_LAT_PRECHARGE_RELAY = 1;  // turn on precharge relay.

	MoveDataFromEEPromToRAM();

	InitCNModule();
	InitializeThrottleAndCurrentVariables();
	InitPIStruct();
	InitADAndPWM();		// Now the A/D is triggered by the pwm period, and the PWM interrupt is enabled.
	InitQEI();
	InitUART2();  // Now the UART is running.
	
	for (i = 0; i < 256; i++) {
		now = counter10k;
		while (counter10k == now) {
			ClrWdt();
		} // a new A/D value will be available.
		localCurrent1 = ADCurrent1;
		localCurrent2 = ADCurrent2;
		vRef1Sum += (long)localCurrent1;
		vRef2Sum += (long)localCurrent2;
	}
	vRef1 = (vRef1Sum >> 8);
	vRef2 = (vRef2Sum >> 8);
	if (vRef1 > 512 + 50 || vRef1 < 512 - 50 || vRef2 > 512 + 50 || vRef2 < 512 - 50) {
 		faultBits |= VREF_FAULT;
	}
	DelayTenthsSecond(savedValues.prechargeTime + 1);  // Make sure at least 5 time constants for precharge time!!
	O_LAT_CONTACTOR = 1;  // close main contactor.
	DelayTenthsSecond(2);
#ifndef OREGON_MOTOR_CONTROLLER
	O_LAT_PRECHARGE_RELAY = 0;  // open precharge relay once main contactor is closed.
#endif
	ClearAllFaults();	// The flip flop and desaturation detection faults start up in an unknown state. clear them.
	U2STAbits.OERR = 0; // ClearReceiveBuffer();
	ShowMenu(); 	// serial show menu.
	while(1) {
		ProcessCommand();  // If there's a command to be processed, this does it.  haha.
		// if myDataStream.period not zero display data stream at specified interval
		if (TMR1 > 115) {
			counter1k++;
			TMR1 = 0;
		}
		if (myDataStream.period) {
			if ((counter1k - myDataStream.timeOfLastTransmission) >= myDataStream.period) {
				GrabDataSnapshot();
				if (myDataStream.showStreamOnce) {
					myDataStream.period = 0;  // Show it once and then stop the stream.
				}
				// myDataStream.period mS passed since last time, adjust myDataStream.timeOfLastTransmission to trigger again
				myDataStream.timeOfLastTransmission = counter1k;
				StreamData(); // 
			}
		}
		if (myPI.testFinished) {
			myPI.testFinished = 0;
			if (myPI.testFailed) {
				TransmitString("No values passed the test.\r\n");
				TransmitString("Try the following:\r\n");
				TransmitString("pi-ratio 63\r\n");
				TransmitString("run-pi-test\r\n");
				TransmitString("pi-ratio 64\r\n");
				TransmitString("run-pi-test\r\n");
				TransmitString("pi-ratio 65\r\n");
				TransmitString("run-pi-test\r\n");
				TransmitString("Just keep going.  If nothing passes the test after you have tried pi-ratio all the way to, say, 200,\r\n");
				TransmitString("it may be that the PI test is too stringent for your motor.  Email me at paulandsabrinasevstuff@gmail.com.\r\n");
			}
			else {
				TransmitString("The test was a success.  If you are staying with this bus voltage, type 'save':\r\n");				
				TransmitString("If you change your bus voltage, you should rerun the command 'run-pi-test'.\r\n");
			}
		}
		else if (myRotorTest.testFinished) {
			myRotorTest.testFinished = 0;
			if (myRotorTest.maxTestSpeed < 32) { // it should be WAY  faster than this.
				TransmitString("Your rotor test failed.\r\n");				
			}
			else {
				TransmitString("Your rotor test was a success!  Type 'config' to see the new rotor time constant.\r\n");
				TransmitString("To save the newly found rotor time constant, type 'save'.\r\n");
			}
		}
		else if (myAngleOffsetTest.testFinished)  {
			myAngleOffsetTest.testFinished = 0;
			if (myAngleOffsetTest.testFailed == 1) {
				TransmitString("Your angle offset test failed.\r\n");				
			}
			else {
				TransmitString("Your angle offset test was a success!  Type 'config' to see the new angle offset.\r\n");
				TransmitString("To save the newly found angle offset, type 'save'.\r\n");
			}
		}
		else if (myMotorSaliencyTest.testFinished) {
			myMotorSaliencyTest.testFinished = 0;
			TransmitString("Motor Saliency Test Time (lower is better): ");
			// 0         1         2         3         4         5
			// 012345678901234567890123456789012345678901234567890123456789
			// xxxxx\r\n
			strcpy(intString,"xxxxx\r\n");
			u16_to_str(&intString[0], myMotorSaliencyTest.elapsedTime, 5);	
			TransmitString(intString);
		}

		if (I_PORT_GLOBAL_FAULT == 0) {
			if (I_PORT_UNDERVOLTAGE_FAULT == 0) {
				faultBits |= UNDERVOLTAGE_FAULT;
			}
//			for (i = 0; i < 10000; i++) {
//				Delay1uS();
//			}
//			faultBits &= ~OVERCURRENT_FAULT;			
//			if (((faultBits & UNDERVOLTAGE_FAULT) == 0) && ((faultBits & DESAT_FAULT) == 0))
//				ClearFlipFlop();
		}
		if (faultBits & STARTUP_FAULT) {
			if (throttle == 0) { // Make sure throttle is zero before allowing things to start.
				faultBits &= ~STARTUP_FAULT;
			}
		}
		if (faultBits != 0) {
			if ((faultBits & 1) && (notShownFaultYet & 1)) {
				TransmitString("Throttle out of range! Is it unplugged?\r\n");
				notShownFaultYet &= ~1;
			}
			if ((faultBits & 2) && (notShownFaultYet & 2)) {
				TransmitString("Desaturation Detection Fault!  That's actually pretty bad.\r\n");
				notShownFaultYet &= ~2;
			}
			if ((faultBits & 4) && (notShownFaultYet & 4)) {
				TransmitString("UART fault.  Is the serial cable unplugged?\r\n");
				notShownFaultYet &= ~4;
			}
			if ((faultBits & 8) && (notShownFaultYet & 8)) {
				TransmitString("Undervoltage Fault.  Either your 5v or 24v supply dropped too low.\r\n");
				notShownFaultYet &= ~8;
			}
			if ((faultBits & 16) && (notShownFaultYet & 16)) {
				TransmitString("Overcurrent fault.  The hardware overcurrent protection just came on and saved the day.\r\n");
				notShownFaultYet &= ~16;
			}
			if ((faultBits & 32) && (notShownFaultYet & 32)) {
				TransmitString("Current sensor fault.  Is a current sensor unplugged?\r\n");
				notShownFaultYet &= ~32;
			}
			if ((faultBits & 64) && (notShownFaultYet & 64)) {
				TransmitString("High pedal lockout fault.  Ignore this for now.  It's not really a fault.  haha.\r\n");
				notShownFaultYet &= ~64;
			}
			if ((faultBits & 128) && (notShownFaultYet & 128)) {
				TransmitString("Rotor flux angle fault.  something weird happened with the rotor flux angle.\r\n");
				notShownFaultYet &= ~128;
			}
			if ((faultBits & 256) && (notShownFaultYet & 256)) {
				TransmitString("There was some hardware caused fault, not originating on the microcontroller (not set by me!\r\n");
				notShownFaultYet &= ~256;
			}
			if ((faultBits & 512) && (notShownFaultYet & 512)) {
				TransmitString("P I Overflow fault.  It's really not tracking IdRef and IqRef well at all.\r\n");
				notShownFaultYet &= ~512;
			}
			if ((faultBits & 1024) && (notShownFaultYet & 1024)) {
				TransmitString("PDC Fault.  I think I got rid of this one.\r\n");
				notShownFaultYet &= ~1024;
			}
			if ((faultBits & 2048) && (notShownFaultYet & 2048)) {
				TransmitString("Motor Overspeed Fault.  The motor RPM got too high!\r\n");
				notShownFaultYet &= ~2048;
			}
			if ((faultBits & 4096) && (notShownFaultYet & 4096)) {
				TransmitString("Magnetizing Current fault.  It got crazy high for some reason.\r\n");
				notShownFaultYet &= ~4096;
			}
			if ((faultBits & 8192) && (notShownFaultYet & 8192)) {
				TransmitString("Encoder cable unplugged.  I'm not using this one at the moment.\r\n");
				notShownFaultYet &= ~8192;
			}
			if ((faultBits & 16384) && (notShownFaultYet & 16384)) {
				TransmitString("\r\nI bet you just typed 'off'  haha..\r\n");
				notShownFaultYet &= ~16384;
			}
		}
		// let the interrupts take care of the rest...
		ClrWdt();  // kick the watchdog.  haha.  That's a Fran original.
 	}
}

//---------------------------------------------------------------------
// The ADC sample and conversion is triggered by the PWM period.
//---------------------------------------------------------------------
// This runs at 9.997kHz.
void __attribute__ ((__interrupt__,auto_psv)) _ADCInterrupt(void) {
	static volatile unsigned int rotorFluxAnglePlus90 = 0;
	static volatile long temperatureSum = 0;
	static volatile unsigned int elapsedTimeInterrupt = 0;
	static volatile int throttleCounter = 0;
	static volatile int cos_theta_times32768 = 0;
	static volatile int sin_theta_times32768 = 0;
	static volatile int tempVd = 0;
	static volatile int tempVq = 0;
	static volatile int temp = 0;
	static volatile int rampRate = 1;
	static volatile unsigned int startTimeInterrupt = 0;
	static volatile long batteryCurrentLong = 0;
	static volatile long vBetaSqrt3_times32768 = 0;
	static volatile long v_alpha_times32768 = 0;
	static volatile int revCounter = 0;	// revCounter increments at 10kHz.  When it gets to 78, the number of ticks in POSCNT is extremely close to the revolutions per seoond * 16.
								// So, the motor mechanical speed will be computed every 1/128 seconds, and will have a range of [0, 3200], where 3200 corresponds to 12000rpm.	
	static volatile long int ticksPerSecInstantaneous = 0;
	static volatile long int ticksPerSecAverage_times256 = 0;
	static volatile long int ticksPerSecAverage = 0;
	static volatile int diffPos = 0;
	static volatile int sign = 1;
	static volatile int bigArrayCounter = 0;

	startTimeInterrupt = TMR4;

    IFS0bits.ADIF = 0;  	// Interrupt Flag Status Register. Pg. 142 in F.R.M.
	// ADIF = A/D Conversion Complete Interrupt Flag Status bit.  
	// ADIF = 0 means we are resetting it so that an interrupt request has not occurred.

	counter10k++;	

	switch(savedValues.motorType) {
		case AC_INDUCTION_MOTOR:	// use these on induction motors.
			// so dense... so glorious.  Covers positive and negative RPM for ACIM only.
			// 
			revCounter++;
			if (revCounter >= revCounterMax) { // 512 ticks per revolution for encoder.
				RPS_times16 = POSCNT;	// if POSCNT is 0x0FFFF due to THE MOTOR GOING BACKWARDS, RPS_times16 would be -1, since it's of type signed short.  So, it's all good.  Negative RPM is covered.
				POSCNT = 0;
				revCounter = 0;
				if (RPS_times16 > maxRPS_times16) {  //
					faultBits |= OVERSPEED_FAULT;
				}
				else if (RPS_times16 < -maxRPS_times16) {
					faultBits |= OVERSPEED_FAULT;
				}
				else {
					faultBits &= ~OVERSPEED_FAULT;
				}
			}
			ComputeRotorFluxAngle();
		break;
		case TOYOTA_MGR_MOTOR:	// Toyota MGR hybrid motor does 2 electrical revolutions per 1 resolver revolution.
			temp = POSCNT;			// it will be in [0,1023).  4 times the 256 ticks coming from the resolver to encoder board.  Two electrical revolutions per index pulse.  It's my resolver to encoder board, so I ought to know!!!  haha.
			if (temp < 10000) { // I initialize POSCNT to an absurdly big value (15000).  It won't be in that range for long though.  But during this time, it could vary from around 14500 up to 15500.  As soon as an index pulse comes along, it will forever be trapped in [0, 511].
				myFirstEncoderIndexPulseTest.testRunning = 0; // poscnt is in [0,1023].  But the actual electrical angle is in [0,511]
				temp += myAngleOffsetTest.currentAngleOffset; 	// currentAngleOffset was found back when the motor had to be configured.  It is normalized to units of [0,511] electrical "degrees". 
			}
			else {  // there hasn't been an index pulse yet.  
				temp += myFirstEncoderIndexPulseTest.currentAngleOffset; 	// currentAngleOffset was found back when the motor had to be configured.  It is in units of [0,511] electrical "degrees". 
			}
			temp &= 511;		// Now, rotorFluxAngle is once again inside [0,511].
			rotorFluxAngle = temp;  // rotorFluxAngle can't be negative.
				// There are NUM_POLE_PAIRS index pulses per mechanical revolution when using the resolver to encoder board ONLY FOR THE LEAF MOTOR!!!!  For the toyota motor, there are 2

			// permanent magnet AC motor mechanical speed computation.
			// do this later.
			poscnt = POSCNT;
			diffPos = poscnt - poscntOld; // if this is > 512, make it diffPos - 1024. That way, let's say poscnt just went from 0 to 1023 (negative rotation).  We can call that diffPos = -1.
			poscntOld = poscnt;
			if (diffPos > 512) { // assume negative rotation.  
				diffPos = diffPos - 1024;
			}
			else if (diffPos < -512) { // For example, let's say poscntOld was 511, and now poscnt is 0 (positive rotation).  We want diffPos to be 1.  So, we do diffPos + 512.
				diffPos = diffPos + 1024; // 
			}
			ticksPerSecInstantaneous = __builtin_mulss(10000,(int)diffPos);  // This will be very erratic.  Wild swings, so smooth it out below with a filter that takes almost 1/50sec to converge to within 99.9%.
			ticksPerSecAverage_times256   	//= (63*ticksPerSecAverage_times64)/64 + 1*ticksPerSecInstantaneous) / 64 * 64;
									 		//= (64*(ticksPerSecAverage_times64/64) - 1*ticksPerSecAverage + 1*ticksPerSecInstantaneous) / 64 * 64
									= ticksPerSecAverage_times256 - ticksPerSecAverage + (long)ticksPerSecInstantaneous;
			ticksPerSecAverage = ticksPerSecAverage_times256 >> 8;
			// Below line assumes 512 ticks per electrical revolution.  The resolver to encoder has 256 ticks, and then I do 4 times the resolution.  But then the stupid toyota MGR does 2 electrical revolutions per index pulse!!  So, 1024 ticks per 2 electrical revolutions.  So, 512 ticks per electrical revolution.
			RPS_times16 = __builtin_divsd((long)ticksPerSecAverage, (int)(savedValues2.numberOfPolePairs << 5));	//  x ticks/sec * mechanicalRevolutions/(polePairs * 512 ticks) = mechanicalRevolutions/sec.  16mechanicalRev/(16sec) = RPS_times16.  There are 512 ticks per electrical revolution with my resolver to encoder after the resolution quadrupling that happens when I intiialize the quadrature encoder interface.
		break;
		case NISSAN_LEAF_MOTOR:
			temp = POSCNT;		// it will be in [0,512).  It's my resolver to encoder board, so I ought to know!!!  haha.
		
			if (temp < 10000) { // I initialize POSCNT to an absurdly big value (15000).  It won't be in that range for long though.  But during this time, it could vary from around 14500 up to 15500.  As soon as an index pulse comes along, it will forever be trapped in [0, 511].
				myFirstEncoderIndexPulseTest.testRunning = 0;
				poscnt += myAngleOffsetTest.currentAngleOffset; 	// currentAngleOffset was found back when the motor had to be configured.  It is in units of [0,511] electrical "degrees". 
				temp &= 511;		// Now, rotorFluxAngle is once again inside [0,511].
				rotorFluxAngle = temp;  // rotorFluxAngle can't be negative.
				// There are NUM_POLE_PAIRS index pulses per mechanical revolution when using the resolver to encoder board WITH THE LEAF MOTOR!!  The friggen toyota MGR has 2 index pulses in 4 electrical revolutions, and in 1 mechanical revolution.
			}
			else {  // there hasn't been an index pulse yet.  
				temp += myFirstEncoderIndexPulseTest.currentAngleOffset; 	// currentAngleOffset was found back when the motor had to be configured.  It is in units of [0,511] electrical "degrees". 
				temp &= 511;		// Now, rotorFluxAngle is once again inside [0,511].
				rotorFluxAngle = temp;  // rotorFluxAngle can't be negative.
			}
			poscnt = POSCNT;		// poscnt is used elsewhere in the program.  Don't modify it like you did with 'temp'.
			diffPos = poscnt - poscntOld; // if this is > 256, make it diffPos - 512. That way, let's say poscnt just went from 0 to 511 (negative rotation)
			poscntOld = poscnt;
			if (diffPos > 256) { // assume negative rotation.  For example, maybe poscntOld was 0 and now poscnt is 511.  diffPos would be 511.
				diffPos = diffPos - 512;
			}
			else if (diffPos < -256) { // For example, let's say poscntOld was 511, and now poscnt is 0 (positive rotation).  We want diffPos to be 1.  So, we do diffPos + 512.
				diffPos = diffPos + 512;
			}
			ticksPerSecInstantaneous = __builtin_mulss(10000,(int)diffPos);  // This will be very erratic.  Wild swings, so smooth it out below with a filter that takes almost 1/50sec to converge to within 99.9%.
			ticksPerSecAverage_times256   	//= (63*ticksPerSecAverage_times64)/64 + 1*ticksPerSecInstantaneous) / 64 * 64;
									 		//= (64*(ticksPerSecAverage_times64/64) - 1*ticksPerSecAverage + 1*ticksPerSecInstantaneous) / 64 * 64
									= ticksPerSecAverage_times256 - ticksPerSecAverage + (long)ticksPerSecInstantaneous;
			ticksPerSecAverage = ticksPerSecAverage_times256 >> 8;
			// Below line assumes 512 ticks per electrical revolution.  The resolver to encoder has 256 ticks from that board I made, and then I double the resolution when it initizlizes the Quadrature Encoder Interface.
			RPS_times16 = __builtin_divsd((long)ticksPerSecAverage, (int)(savedValues2.numberOfPolePairs << 5));	//  x ticks/sec * mechanicalRevolutions/(polePairs * 512 ticks) = mechanicalRevolutions/sec.  16mechanicalRev/(16sec) = RPS_times16.  There are 512 ticks per electrical revolution with my resolver to encoder after the resolution doubling that happens when I intiialize the quadrature encoder interface.
		break;
		case PERMANENT_MAGNET_MOTOR_WITH_ENCODER:
			// There are 'n' POSCNT values that would work in [0, NUM_ENCODER_TICKS*4], where 'n' is the number of pole pairs.
			// POSCNT is in [0, NUM_ENCODER_TICKS*4].  An index pulse resets POSCNT back to zero, or to MAX_POSCNT if counting down.
			// 
			//			temp = POSCNT;
			//			tempLong = __builtin_muluu((unsigned int)temp,(unsigned int)savedValues2.numberOfPolePairs) << 7; // max encoder ticks is 4096, max pole pairs is 128.  Well, the product of max encoder ticks and number of pole pairs must be <= 4096*128, since (4096*4)*128*512 = 2^32.  
			//			temp = __builtin_divud((unsigned long)tempLong, (unsigned int)savedValues2.encoderTicks);
			//			temp += myAngleOffsetTest.currentAngleOffset;
			//			rotorFluxAngle = temp & 511;
			// Now find RPS_times16.
		break;
	}
	// CH0 corresponds to ADCBUF0. etc...
	// CH0=AN7, CH1=AN0, CH2=AN1, CH3=AN2. 
	// AN0 = CH1 = ADThrottle
	// AN1 = CH2 = ADCurrent1
	// AN2 = CH3 = ADCurrent2
	// AN7 = CH0 = ADTemperature

	ADCurrent1 = ADCBUF2;
	ADCurrent2 = ADCBUF3;
	Ia = ADCurrent1;	// CH2 = ADCurrent1
	Ib = ADCurrent2;		// CH3 = ADCurrent2.

	Ia -= vRef1;  // vRef1 is just a constant found at the beginning of the program, approximately = 512, that changes the current feedback from being centered at 512 to centered at 0.  It's specific to current sensor #1.
	Ib -= vRef2;  // vRef2 is just a constant found at the beginning of the program, approximately = 512, that changes the current feedback from being centered at 512 to centered at 0.  It's specific to current sensor #2.
	// So, you must change the interval to [-4096, 4096], so as to match the throttle range below to make feedback comparable with commanded current. So...

	Ia <<= 4;	// Ia is now in [-4096, 4096] if it was in  [-256, 256].  In other words, if it was in [-2*LEM Rating, 2*LEM Rating]. 
	Ib <<= 4;   // Ib is now in [-4096, 4096] if it was in  [-256, 256].  In other words, if it was in [-2*LEM Rating, 2*LEM Rating]. 
	Ic = -Ia - Ib;
	Ib_times2 = (Ib << 1);

	rawThrottle = ADCBUF1;
	throttleSum += rawThrottle;
	temperatureSum += ADCBUF0;
	batteryCurrentLong = __builtin_mulss(Ia,pdc1) + __builtin_mulss(Ib,pdc2) + __builtin_mulss(Ic,pdc3);  // batteryCurrent is in [-4096*sqrt(3)/2, 4096*sqrt(3)/2] = [-3547, 3547].
	batteryCurrentSum += batteryCurrentLong;
	throttleCounter++;
	if (throttleCounter >= 128) {
		throttleCounter = 0;
		throttle = (throttleSum >> 7);  // in [0, 1023].
		temperatureBasePlate = (temperatureSum >> 7); // in [0,1023]
		if (temperatureBasePlate < 147) temperatureBasePlate = 147; // clamp it so you get an integer when converting to the variable resistance.  This assumes +5v -- 1k -- 10k thermistor --- 4.7k --- ground.  P/N: NTCALUG03A103G
		batteryCurrentNormalized = __builtin_divsd(batteryCurrentSum >> 7,MAX_DUTY);  // 
		throttleSum = 0;
		temperatureSum = 0;
		batteryCurrentSum = 0;
	
		// Is there a throttle fault?
		if (throttle <= savedValues.throttleFaultPosition) {
			faultBits |= THROTTLE_FAULT;
		}
		if (savedValues2.throttleType == 1) {
			throttle = 1023 - throttle;  // invert it.
		}
		// I think I'll work my way left to right.  ThrottleFaultVoltage < ThrottleMaxRegen < ThrottleMinRegen < ThrottleMin < ThrottleMax 
		if (throttle < savedValues.maxRegenPosition) {  //First clamp it below.
			throttle = savedValues.maxRegenPosition;	
		}
		else if (throttle > savedValues.maxThrottlePosition) {	// And clamp it above!  
			throttle = savedValues.maxThrottlePosition;
		}
		if (throttle < savedValues.minRegenPosition) {  // It's in regen territory.  Map it from [savedValues.maxThrottleRegen, savedValues.minThrottleRegen) to [-maxMotorCurrentNormalizedRegen, 0)
			throttle -= savedValues.minRegenPosition;  // now it's in the range [maxThrottleRegen - minThrottleRegen, 0)
			throttle =	-__builtin_divsd(((long)throttle) * maxMotorCurrentNormalizedRegen, savedValues.maxRegenPosition - savedValues.minRegenPosition);
		}
		else if (throttle <= savedValues.minThrottlePosition) { // in the dead zone!
			throttle = 0;
		}
		else { // <= throttle max is the only other option!  Map the throttle from (savedValues.minThrottlePosition, savedValues.maxThrottlePosition] to (0,maxMotorCurrentNormalized]
			throttle -= savedValues.minThrottlePosition;
			throttle = __builtin_divsd(__builtin_mulss(throttle, maxMotorCurrentNormalized), savedValues.maxThrottlePosition - savedValues.minThrottlePosition);
		}
		if (temperatureBasePlate > THERMAL_CUTBACK_START) {  // Force the throttle to cut back.
			temperatureMultiplier = (temperatureBasePlate - THERMAL_CUTBACK_START) >> 3;  // 0 THROUGH 7.
			if (temperatureMultiplier >= 7)
				temperatureMultiplier = 0;
			else {
				// temperatureMultiplier is now 6 to 0 (for 1/8 to 7/8 current)
				temperatureMultiplier = 7 - temperatureMultiplier;
				// temperatureMultiplier is now 1 for 1/8, 2 for 2/8, ..., 7 for 7/8, etc.
			}
		}
		else {
			temperatureMultiplier = 8;	// Allow full throttle.
		}
		currentRadiusRefRef = __builtin_mulss(throttle,temperatureMultiplier) >> 3;  // scale back the current command based on temperature.
		if (RPS_times16 < 8) {  // if less than 0.5 rev per second, make sure there's no regen.
			if (currentRadiusRefRef < 0) currentRadiusRefRef = 0;
		}
		if (currentRadiusRefRef < 0) {
			currentRadiusRefRef = -currentRadiusRefRef;
			sign = -1;
		}
		else {
			sign = 1;
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////
		// Keep battery amps in [-savedValues.maxBatteryAmpsRegen, savedValues.maxBatteryAmps] //////////	
		if (batteryCurrentNormalized > maxBatteryCurrentNormalized) { // maxBatteryCurrentNormalized is positive.  Computed in "ProcessCommand", each time a new savedValues.maxBatteryAmps is found.
			// averageDuty = (pdc1+pdc2+pdc3)/3.  But division is slow.  So, do this instead:
			// averageDuty = 65536 * ((pdc1+pdc2+pdc3)/3) / 65536
			// averageDuty = 21845 * (pdc1+pdc2+pdc3) / 65536
			// averageDuty = (21845 * (pdc1+pdc2+pdc3)) >> 16
			averageDuty = __builtin_mulss(21845, pdc1+pdc2+pdc3) >> 16;	// avoiding divide by 3.  HAHA.  
			if (averageDuty > 0) {
				temp = __builtin_divsd(maxBatteryCurrentNormalized,averageDuty); // 
				if (currentRadiusRefRef > temp) {
					currentRadiusRefRef = temp;
				}
			}
		}
		else if (batteryCurrentNormalized < -maxBatteryCurrentNormalizedRegen) { // maxRegenCurrent is negative.  Computed in "InitializeThrottleAndCurrentVariables()".
			// averageDuty = (pdc1+pdc2+pdc3)/3.  But division is slow.  So, do this instead:
			// averageDuty = 65536 * ((pdc1+pdc2+pdc3)/3) / 65536
			// averageDuty = 21845 * (pdc1+pdc2+pdc3) / 65536
			// averageDuty = (21845 * (pdc1+pdc2+pdc3)) >> 16
			averageDuty = __builtin_mulss(21845, pdc1+pdc2+pdc3) >> 16;	// avoiding divide by 3.  HAHA.  
			if (averageDuty > 0) {
				temp = -__builtin_divsd(maxBatteryCurrentNormalizedRegen,averageDuty); // This is the first try at IqRefRef.  Later, other values will be calculated based on throttle.  Keep the one closest to zero.
				if (currentRadiusRefRef < temp) {
					currentRadiusRefRef = temp;  // if it was more negative, then make it smaller in magnitude, 'cuz there's too much friggen current going into the batteries!!!
				}
			}
		}

		if (myRotorTest.testRunning) {  // I need this to run for say, 2 seconds, and then record the RPM.
			currentRadiusRefRef = 300;
			if (counter10k - myRotorTest.startTime > 20000) {  // 2 seconds.
				myRotorTest.startTime = counter10k;
				if (RPS_times16 > myRotorTest.maxTestSpeed) {
					myRotorTest.maxTestSpeed = RPS_times16; // save the best speed so far.
					myRotorTest.bestTimeConstantIndex = myRotorTest.timeConstantIndex;
				}
				myRotorTest.timeConstantIndex++;
				if (myRotorTest.timeConstantIndex >= MAX_ROTOR_TIME_CONSTANT_INDEX) {
					savedValues2.rotorTimeConstantIndex = myRotorTest.bestTimeConstantIndex;
					myRotorTest.testRunning = 0;
					myRotorTest.testFinished = 1;
					currentRadiusRefRef = 0;
				}
			}
		}
		else if (myFirstEncoderIndexPulseTest.testRunning) {
			// IqRefRef & IdRefRef are already found from measuring throttle.
			if (counter10k - myFirstEncoderIndexPulseTest.startTime > 2500) {  // 0.25 seconds per attempt.  2 seconds to try all the way around the circle if it's broken into 8 pieces.  Do you only have to go half way around?
				myFirstEncoderIndexPulseTest.startTime = counter10k;
				if (RPS_times16 == 0) { // move to the next angle offset. This one isn't producing movement with the motor.
					myFirstEncoderIndexPulseTest.currentAngleOffset += 64;  // move 1/8 of a revolution, to see if you can find a better guess at the rotor flux angle.  It starts in a unknown state.
					myFirstEncoderIndexPulseTest.currentAngleOffset &= 511;
				}
			}
		}

		// Now that currentRadiusRefRef has been established based on throttle position, temperature, etc. we need to decide how to break it down into IdRefRef and IqRefRef.  That will depend on the type of motor we are using.
		if (savedValues.motorType != AC_INDUCTION_MOTOR) { // 2 & 3 are permanent magnet types.  1 is induction..
//			K = saliencyConstant[myMotorSaliencyTest.KArrayIndex]; 
//			currentRadiusRefRefOverSqrt2 = __builtin_mulus(46341u,currentRadiusRefRef) >> 16;
//			IdRefRef = -K + unknownTriangleLeg(K, currentRadiusRefRefOverSqrt2);
			if (myAngleOffsetTest.testRunning) {
				IdRefRef = 300;
				IqRefRef = 0;
				if (counter10k - myAngleOffsetTest.startTime > 5000) {  // 0.5 seconds. 
					myAngleOffsetTest.startTime = counter10k;
					if (RPS_times16 == 0) {
						myAngleOffsetTest.testRunning = 0;
						myAngleOffsetTest.testFinished = 1;
						myAngleOffsetTest.testFailed = 0;
						IdRefRef = 0;
						IqRefRef = 0;
					}
					else {
						if (myAngleOffsetTest.currentAngleOffset < 511) {
							myAngleOffsetTest.currentAngleOffset++;
						}
						else {
							myAngleOffsetTest.testRunning = 0;
							myAngleOffsetTest.testFinished = 1;
							currentRadiusRefRef = 0;
						}
					}
					savedValues2.angleOffset = myAngleOffsetTest.currentAngleOffset;
				}
			}
			else {
				#ifdef OREGON_MOTOR_CONTROLLER
					IdRefRef = 0; // Let Id be this percent of the radius.  So, that looks like radius * that / 1024.
					IqRefRef = currentRadiusRefRef;
					IqRefRef *= sign;
				#else
					IdRefRef = __builtin_mulss(-myMotorSaliencyTest.KArrayIndex,currentRadiusRefRef) >> 10; // Let Id be this percent of the radius.  So, that looks like radius * that / 1024.
					IqRefRef = unknownTriangleLeg(currentRadiusRefRef, IdRefRef);
					IqRefRef *= sign;
				#endif
			}
		}
		else {
			IdRefRef = currentRadiusRefRef;				// change this later so that IdRefRef^2 + IqRefRef^2 = currentRadiusRefRef^2.
			IqRefRef = sign*currentRadiusRefRef;
		}
	}

	if (myPI.testRunning) {
		rotorFluxAngle = 0;
		if ((counter10k - myPI.previousTestCompletionTime) < 1000) {  // wait 0.1 seconds between tests.  That gives Id and Iq a chance to go back to zero.
			IdRef = 0;
			IqRef = 0;
		}
		else {  // I'm running the PI loop test on a particular Kp and Ki to see if it passes the convergence test below.
			IdRef = 0;  // 512 on a scale of 0 to 4096 corresponds to 75 amps for a LEM Hass 300-s.  Because 4096 means 600amps.
			IqRef = 511;
	
			if (myPI.iteration == 0) {
				myPI.iteration++;
			}
			else {
				if (myPI.error_q < -120/*-80*/) {  // if it overshot the target by 80, move on to the next one.  We don't want overshoot.
					MoveToNextPIValues();
				}
				else if (myPI.error_q > IqRef + 200) {  //  IqRef is a constant 511.  If Iq swung to -200, that's bad.  Move on.  Iq shouldn't go below zero much.
					MoveToNextPIValues();
				}
				else if (myPI.zeroCrossingIndex == -1 && myPI.iteration > myPI.maxIterationsBeforeZeroCrossing) {  // myPI.zeroCrossingIndex == -1 means it hasn't crossed zero yet.
					MoveToNextPIValues();  // CONVERGENCE TOO SLOW!!!  Move on!
				}
				else if (myPI.error_q > 120/*80*/ && myPI.zeroCrossingIndex >= 0) {  // it already crossed zero, but now is way back up again.  This is oscillation.  move on!
					MoveToNextPIValues();
				}
				else {
					if (myPI.error_q <= 0 && myPI.zeroCrossingIndex == -1) {  // if it's crossing zero for the first time, record the location.
						myPI.zeroCrossingIndex = myPI.iteration;
					}
					myPI.iteration++;
					if (myPI.iteration >= 400) {  // 400 iterations of the PI loop happened, and it passed all the rigorous requirements.  Save Kp & Kq constants.  They are keepers!
						savedValues.Kp = myPI.Kp;
						savedValues.Ki = myPI.Ki;
						myPI.testRunning = 0;  // You found a good PI value, so quit hunting!
						myPI.testFailed = 0;
						myPI.testFinished = 1;
					}
				}
				if (myPI.Kp > 20000) {  // 
					myPI.testRunning = 0;
					myPI.testFailed = 1;
					myPI.testFinished = 1;
				}
			}
		}
	}
	else if (myMotorSaliencyTest.testRunning) {
		currentRadiusRefRef = 200;
//		K = saliencyConstant[myMotorSaliencyTest.KArrayIndex]; 
//		currentRadiusRefRefOverSqrt2 = __builtin_mulus(46341u,currentRadiusRefRef) >> 16;
		IdRefRef = __builtin_mulss(-myMotorSaliencyTest.KArrayIndex,currentRadiusRefRef) >> 10; // try various negative values...  from 0 down to whatever the crap you want.
		
//		IdRefRef = -K + unknownTriangleLeg(K, currentRadiusRefRefOverSqrt2);
		IqRefRef = unknownTriangleLeg(currentRadiusRefRef, IdRefRef);

		if (voltageDiskExceeded) {
			myMotorSaliencyTest.elapsedTime = counter10k - myMotorSaliencyTest.startTime;
			currentRadiusRefRef = 0;
			myMotorSaliencyTest.testRunning = 0;
			myMotorSaliencyTest.testFinished = 1;
			currentRadiusRefRef = 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Clarke transform:
	//  	First, take the 3 vectors, 120 degrees apart, and add them to
	// 		get a new vector, and project that vector onto the x and y axis.  The x-axis component is called i_alpha.  y-axis component is called i_beta.
	// clarke transform, scaled down by 2/3 to simplify it:
	// i_alpha = i_a
	// 1/sqrt(3) * 2^16 = 37837

	i_alpha = Ia;
	i_beta = __builtin_mulsu((int)(Ib_times2 + Ia), 37837u) >> 16;  // 1/sqrt(3) * (i_a + 2 * Ib).  

	// End of clarke transform.
	////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Park transform:
	// rotorFluxAngle is normalized to something in [0, 511] electrical "degrees".
	// sin(theta + 90 degrees) = cos(theta).
	// I want the 2 angles to be in [0, 511] so I can use the lookup table.
	rotorFluxAnglePlus90 = ((rotorFluxAngle + 128) & 511);  // To advance 90 degrees on a scale of 0 to 511, you add 128, 
															// and then do "& 511" to make it wrap around if overflow occurred.
	
	cos_theta_times32768 = _sin_times32768[rotorFluxAnglePlus90];  // 
	sin_theta_times32768 = _sin_times32768[rotorFluxAngle];  // 
	// Park Transform:
	// Id = Ialpha*cos(theta) + Ibeta*sin(theta)
	// Iq = -Ialpha*sin(theta) + Ibeta*cos(theta)
	Id = (__builtin_mulss((int)i_alpha, (int)cos_theta_times32768) + __builtin_mulss((int)i_beta, (int)sin_theta_times32768)) >> 15; 	
	Iq = (__builtin_mulss((int)(-i_alpha), (int)sin_theta_times32768) + __builtin_mulss((int)i_beta,(int)cos_theta_times32768)) >> 15; 


	/////////////////////////////////////////////////////////////////////////////////////////////////
	// PI Loop:
	myPI.error_q = IqRef - Iq;
	myPI.error_d = IdRef - Id;
	
	myPI.errorSum_q += myPI.error_q - myPI.clampErrorVq;
	myPI.errorSum_d += myPI.error_d - myPI.clampErrorVd;
	
//	if (IdRef == 0 && IqRef == 0) {  // this causes overcurrent faults when going back to zero throttle from nonzero throttle with permanent magnet motors.  It works great in the case of an induction motor.  oh well.
//		myPI.pwm_d = 0;
//		myPI.errorSum_d = 0;
//		myPI.error_d = 0;
//		myPI.clampErrorVd = 0;
//		myPI.pwm_q = 0;
//		myPI.errorSum_q = 0;
//		myPI.error_q = 0;
//		myPI.clampErrorVq = 0;
//	}
//	else {
		myPI.pwm_d = (myPI.Kp * myPI.error_d) + (myPI.Ki*myPI.errorSum_d);
		myPI.pwm_q = (myPI.Kp * myPI.error_q) + (myPI.Ki * myPI.errorSum_q);
//	}

	if (myPI.pwm_d > (MAX_VD_VQ << 13)) {
		myPI.pwm_d = (MAX_VD_VQ << 13);
		faultBits |= PI_OVERFLOW_FAULT;
	}
	else if (myPI.pwm_d < ((-MAX_VD_VQ) << 13)) {
		myPI.pwm_d = ((-MAX_VD_VQ) << 13);		
		faultBits |= PI_OVERFLOW_FAULT;
	}
	if (myPI.pwm_q > (MAX_VD_VQ << 13)) {
		myPI.pwm_q = (MAX_VD_VQ << 13);
		faultBits |= PI_OVERFLOW_FAULT;
	}
	else if (myPI.pwm_q < ((-MAX_VD_VQ) << 13)) {
		myPI.pwm_q = ((-MAX_VD_VQ) << 13);		
		faultBits |= PI_OVERFLOW_FAULT;
	}

	Vd = myPI.pwm_d >> 13;
	Vq = myPI.pwm_q >> 13;
	tempVd = Vd;
	tempVq = Vq;

	ClampVdVq();
	myPI.clampErrorVd = tempVd - Vd;
	myPI.clampErrorVq = tempVq - Vq;
	//	if (maxClampErrorVd < myPI.clampErrorVd) maxClampErrorVd = myPI.clampErrorVd;
	//	if (minClampErrorVd > myPI.clampErrorVd) minClampErrorVd = myPI.clampErrorVd;
	//	if (maxClampErrorVq < myPI.clampErrorVq) maxClampErrorVq = myPI.clampErrorVq;
	//	if (minClampErrorVq > myPI.clampErrorVq) minClampErrorVq = myPI.clampErrorVq;

	// should I have it go instantly to IqRefRef if IqRefRef is smaller in magnitude?
	if (IqRef < IqRefRef) {
		IqRef += rampRate;  // I want it to be a laxidaisical drift back so as to avoid harsh PI loop clamping.
		if (IqRef > IqRefRef) {
			IqRef = IqRefRef;
		}
	}
	else if (IqRef > IqRefRef) {
		IqRef -= rampRate;
		if (IqRef < IqRefRef) {
			IqRef = IqRefRef;
		}
	}

	// should I have it go instantly to IdRefRef if IdRefRef is smaller in magnitude?
	if (IdRef < IdRefRef) {
		IdRef += rampRate;
		if (IdRef > IdRefRef) {
			IdRef = IdRefRef;
		}
	}
	else if (IdRef > IdRefRef) {
		IdRef -= rampRate;
		if (IdRef < IdRefRef) {
			IdRef = IdRefRef;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Inverse Park Transform:
	//	v_alpha_times32768 = (Vd*cos_theta_times32768 - Vq*sin_theta_times32768);	// shift right 15 because sin and cos have been shifted left by 15.
	//	vBeta =  (Vd*sin_theta_times32768 + Vq*cos_theta_times32768) >> 15;	//
	v_alpha_times32768 = __builtin_mulss((int)Vd, (int)cos_theta_times32768) - __builtin_mulss((int)Vq, (int)sin_theta_times32768);
	v_beta = (__builtin_mulss((int)Vd, (int)sin_theta_times32768) + __builtin_mulss((int)Vq, (int)cos_theta_times32768)) >> 15;
	//////////////////////////////////////////////////////////////////////////////////
	// Now do the inverse Clarke transform, with a scaling factor of 3/2 to simplify it.
	//  Va = v_alpha
	//	Vb = 1/2*(-v_alpha + sqrt(3)*vBeta)
	//  Vc = 1/2*(-v_alpha - sqrt(3)*vBeta);
	v_alpha = v_alpha_times32768 >> 15;
	Va = v_alpha; 
	//vBetaSqrt3_times32768 = (56756*vBeta);  // 56756 = sqrt(3)*(2^15).
	vBetaSqrt3_times32768 = __builtin_mulus(56756u, (int)v_beta);  // 56756 = sqrt(3)*(2^15).

	Vb = (-v_alpha_times32768 + vBetaSqrt3_times32768) >> 16;  // scaled up by 2^15, so shift down by 15.  But you also must divide by 2 at the end as part of the inverse clarke.  So, shift down by a total of 16.
	Vc = (-v_alpha_times32768 - vBetaSqrt3_times32768) >> 16;
	///////////////////////////////////////////////////////////////////////////////////
	// SpaceVectorModulation:
	// You now turn Va, Vb, Vc into duties for PDC1, PDC2, PDC3.  

	if (faultBits == 0) {  // check again before poceeding, since there could have been a fault just above.
		SpaceVectorModulation();
	}
	else {
		PDC1 = 0;
		PDC2 = 0;
		PDC3 = 0;
		myPI.error_d = 0l;
		myPI.pwm_d = 0l;  
		myPI.errorSum_d = 0l;
		myPI.error_q = 0l;
		myPI.pwm_q = 0l;  
		myPI.errorSum_q = 0l;		
	}
	elapsedTimeInterrupt = TMR4 - startTimeInterrupt;
}

int unknownTriangleLeg(int c, int b) {
	// 
	// assume that b and c are positive integers.
// This finds a = sqrt(c*c - b*b), where triangle a,b,c is a right triangle, and c is the hypotenuse.
// I have a lookup table for all possible sqrt(1024*1024 - b*b).
// So, let's say c = 512 & b = 75.  Before using the lookup table, you would have to scale things so that c = 1024.  If you double c and b, then you also double the answer.
// so, to get the true answer, you have to cut 'a' in half at the end.
// 
// newC = 1024.  newB = b*1024/c.  a = unknownTriangleLegLookup[newB];
	static volatile int newB = 0;
	static volatile int a = 0;
	if (c == 0) return 0;
	if (b >= c) return 0;
	if (b < 0) b = -b;
	newB = __builtin_divsd(((long)b) << 10, c);
	a = unknownTriangleLegLookup[newB];  // a = sqrt(1024*1024 - newB*newB).
	a = __builtin_mulss(a,c) >> 10; // Now, scale a back to what it should be.  a = a*c / 1024?
	return a;
}

void MoveToNextPIValues() {
	myPI.Kp += myPI.ratioKpKi;
	myPI.Ki += 1;
	myPI.error_d = 0l;
	myPI.pwm_d = 0l;  
	myPI.errorSum_d = 0l;

	myPI.error_q = 0l;
	myPI.pwm_q = 0l;  
	myPI.errorSum_q = 0l;	

	myPI.iteration = 0;
	myPI.zeroCrossingIndex = -1;
	IdRef = 0;
	IqRef = 0;
	myPI.previousTestCompletionTime = counter10k;	
}

void InitPIStruct() {
	myPI.Kp = (long)savedValues.Kp;
	myPI.Ki = (long)savedValues.Ki;
	myPI.error_d = 0l;
	myPI.pwm_d = 0l;  
	myPI.errorSum_d = 0l;

	myPI.error_q = 0l;
	myPI.pwm_q = 0l;  
	myPI.errorSum_q = 0l;

	myPI.testFinished = 0;
	myPI.testFailed = 0;
	myPI.testRunning = 0;
	myPI.ratioKpKi = 62; //savedValues2.ratioKpKi;
	myPI.zeroCrossingIndex = -1; // initialize to -1.
	myPI.iteration = 0; // how many times have you run the PI loop with the same Kp and Ki?  This is used in the PI auto loop tuning.
	myPI.maxIterationsBeforeZeroCrossing = 20; //20
	myPI.previousTestCompletionTime = counter10k;
}

void ComputeRotorFluxAngle() {
	static volatile unsigned int rotorFluxAngle_times128 = 0;  // For fine control.
	static volatile long magCurrChange = 0L;
	static volatile long slipSpeedNumerator = 0L;
	static volatile int angleChange_times128 = 0;
	static volatile long magnetizingCurrentFine = 0L;
	static volatile int magnetizingCurrent = 0;
//	static volatile unsigned long tempLong = 0UL;
	static volatile /*unsigned*/ int temp = 0;

//;	 Physical form of equations:
//;  Magnetizing current (amps):
//;     Imag = Imag + (fLoopPeriod/fRotorTmConst)*(Id - Imag)
//;
//;  Slip speed in RPS:
//;     VelSlipRPS = (1/fRotorTmConst) * (Iq/Imag) / (2*pi)
//;
//;  Rotor flux speed in RPS:
//;     VelFluxRPS = iPoles * VelMechRPS + VelSlipRPS
//;
//;  Rotor flux angle (radians):
//;     AngFlux = AngFlux + fLoopPeriod * 2 * pi * VelFluxRPS

// ****For divide, use this:  	int quot =	 __builtin_divsd(long numerator, int denominator);****
// ****For multiply, use this:  long prod =   __builtin_mulus(unsigned left,  int right);****  or muluu, or mulsu, or mulss.

//; 1.  Magnetizing current (amps):
//;     Imag = Imag + (fLoopPeriod/fRotorTmConst)*(Id - Imag)
//      rotorTimeConstantArray[]'s entries have been scaled up by 2^18 to give more resolution for the rotor time constant.  I scaled by 18, because that allowed incrementing rotor time constant by 0.01 seconds.
//	magnetizingCurrent += ((rotorTimeConstantArray1[myRotorTest.rotorTimeConstantIndex] * (Id - magnetizingCurrent))) >> 18;
///////////////////////////////////////////////////////////////////////////

	magCurrChange = __builtin_mulus((unsigned int)rotorTimeConstantArray1[myRotorTest.timeConstantIndex], (int)(Id - magnetizingCurrent));
	if (magCurrChange > 0) {
		if (magnetizingCurrentFine < MAX_LONG_INT - magCurrChange) {
			magnetizingCurrentFine += magCurrChange;
		}
		else {
			magnetizingCurrentFine = MAX_LONG_INT;
			faultBits |= MC_OVERFLOW_FAULT;
		}
	}
	else {
		if (magnetizingCurrentFine > -MAX_LONG_INT - magCurrChange) {
			magnetizingCurrentFine += magCurrChange;
		}
		else {
			magnetizingCurrentFine = -MAX_LONG_INT;
			faultBits |= MC_OVERFLOW_FAULT;
		}
	}
	magnetizingCurrent = (int)(magnetizingCurrentFine >> 18);
//	magnetizingCurrentAmps_times8 = __builtin_mulss((int)magnetizingCurrent, (int)currentSensorAmpsPerVoltTimes5) >> 11;  // 5/4*currentSensorAmpsPerVolt / 4096 * #ticks = amps.  So amps*8 cancels it down to >>11.

////////////////////////////////////////////////////////////////////////////
//; 2. To Compute Slip speed in RPS:
//;    VelSlipRPS = (1/fRotorTmConst) * (Iq/Imag) / (2*pi)
//     rotorTimeConstantArray2[] entries are 1/fRotorTmConst * 1/(2*pi) * 2^11.  I couldn't scale any higher than 2^11 so as to keep them integers. (this is not 100% true now.  haha.)
//	   VelSlipRPS = (ARRAY[] * Iq) / Imag.
//	   Let slipSpeedNumerator = (ARRAY[] * Iq).  Then, do the bit shift down first, and the divide by magnetizing current afterwards to prevent the loss of resolution.
/////////////////////////////////////////////////////////////////////////////////////////////////////	
	if (magnetizingCurrent == 0) {
		return;  // there is no rotor flux angle, since there is no field.  So, keep the angle the same as it was before??
	}
	else if (Iq == 0) {
		slipSpeedRPS_times16 = 0;  // If the numerator is 0, the whole thing is zero.
	}
	else {  // magnetizingCurrent != 0, slipSpeedNumerator != 0
		slipSpeedNumerator = __builtin_mulus((unsigned int)rotorTimeConstantArray2[myRotorTest.timeConstantIndex], (int)Iq) >> 7; // Must scale down by 2^11 if you want units to be rev/sec.  But that's too grainy.  So, let's only scale down by 2^7 so you get slip speed in rev/sec * 16, rather than just rev/sec
		if (magnetizingCurrent > 0) {
			if (slipSpeedNumerator > 0L) {
				if (slipSpeedNumerator > __builtin_mulss((int)magnetizingCurrent,(int)MAX_SLIP_SPEED_RPS_TIMES16)) {
					slipSpeedRPS_times16 = MAX_SLIP_SPEED_RPS_TIMES16;  // must be positive slip speed
				}
				else {
					slipSpeedRPS_times16 = __builtin_divsd((long)slipSpeedNumerator, (int)magnetizingCurrent);  // must be positive slip speed.
				}
			}
			else { // if (slipSpeedNumerator < 0).  It can't be zero, since that case was already accounted for.
				if (-slipSpeedNumerator > __builtin_mulss((int)magnetizingCurrent,(int)MAX_SLIP_SPEED_RPS_TIMES16)) {
					slipSpeedRPS_times16 = -MAX_SLIP_SPEED_RPS_TIMES16;  // must end with negative slip speed.
				}
				else {
					slipSpeedRPS_times16 = __builtin_divsd((long)slipSpeedNumerator, (int)magnetizingCurrent);  // this is negative result.
				}
			}
		}
		else {  // magnetizingCurrent < 0
			if (slipSpeedNumerator > 0) {  // POS / NEG = NEG.
				if (slipSpeedNumerator > __builtin_mulss((int)(-magnetizingCurrent),(int)MAX_SLIP_SPEED_RPS_TIMES16)) {  //
					slipSpeedRPS_times16 = -MAX_SLIP_SPEED_RPS_TIMES16;  // must be negative slip speed.  pos/neg = neg.
				}
				else {
					slipSpeedRPS_times16 = __builtin_divsd((long)slipSpeedNumerator, (int)magnetizingCurrent);  // must be negative slip speed.
				}
			}
			else {  // slipSpeedNumerator < 0, magnetizingCurrent < 0.  So, slipSpeed will be positive below.
				if (slipSpeedNumerator < __builtin_mulss((int)magnetizingCurrent,(int)MAX_SLIP_SPEED_RPS_TIMES16)) {  // if it's more negative than the right hand side...
					slipSpeedRPS_times16 = MAX_SLIP_SPEED_RPS_TIMES16;  // must end with positive slip speed.  neg / neg = pos.
				}
				else {
					slipSpeedRPS_times16 = __builtin_divsd((long)slipSpeedNumerator, (int)magnetizingCurrent);  // this is positive result.
				}
			}
		}
	}
///////////////////////////////////////////////////////////////////////////////////

//; 3. Rotor flux speed in RPS:
//;    VelFluxRPS = numPolePairs * VelMechRPS + VelSlipRPS
//     RPS_times16 was gloriously found in the main interrupt. It is in [-3200, 3200], which means [-12000RPM, 12000RPM].  I will make sure that normal driving is positive rpm.
//     savedValues2.numberOfPolePairs is 2 in the case of my motor, because the RPM is listed as 1700 with 60 Hz 3 phase input.  1 pole pair would list the rpm as around 3400 with 60 Hz 3 phase input.
//	rotorFluxRPS_times16 = savedValues2.numberOfPolePairs * RPS_times16 + slipSpeedRPS_times16;  // There's no danger of integer overflow for 1 or 2 pole pairs, so just do normal multiply.

		rotorFluxRPS_times16 = savedValues2.numberOfPolePairs*RPS_times16 + slipSpeedRPS_times16;  // There's no danger of integer overflow, so just do normal multiply.  Larger number of pole pairs means lower rpm, so it all evens out.

//;  Rotor flux angle (radians):
//;     AngFlux = AngFlux + fLoopPeriod * 2 * pi * VelFluxRPS
//;  Rotor flux angle (0 to 511 ticks):
//      AngFlux = AngFlux + fLoopPeriod * 512 * VelFluxRPS.  
//   Now, I don't have VelFluxRPS.  Too darn grainy.  I found rotorFluxRPS_times16 above.  So.....
//      AngFlux = AngFlux + fLoopPeriod * 32 * rotorFluxRPS_times16, because 32 * 16 = 512.
//   OK, fLoopPeriod = 0.0001 sec.  I don't want to divide here, so let's do a trick that is much faster.
//   0.0001sec * 32 * 2^24 = 53687.09.  So, I'll just multiply by 53687, and shift down by 2^24 afterwards.  Actually, let's keep 2^7 worth of extra resolution, because angleChange was too grainy before.
//	angleChange_times128 = (53687u * rotorFluxRPS_times16) >> 17;  // must shift down by 24 eventually, but let's keep a higher resolution here.  So, only shift down by 17.  Keeping 7 bits.
	angleChange_times128 = __builtin_mulus(53687u, (int)rotorFluxRPS_times16) >> 17;  // must shift down by 24 eventually, but let's keep a higher resolution here.  So, only shift down by 17.  Keeping 7 bits.
	// angleChange_times128 must be in [-5242, 5242] assuming all the clamping I'm doing above.
	rotorFluxAngle_times128 += (unsigned)angleChange_times128;  // if it overflows, so what.  it will wrap back around.  To go from [0,65536] --> [0,512], divide by by 128.  higher resolution rotor flux angle saved here.
	rotorFluxAngle = (rotorFluxAngle_times128 >> 7);
}

void ClampVdVq() {
	static volatile int VdPos, VqPos, small, big;
	static volatile int i;
	static volatile unsigned int fastDist;
	static volatile unsigned int r;
	static volatile unsigned int scale;
	static volatile int IdRefNew = 0;
	static volatile int IqRefNew = 0;

	if (Vd == 0 && Vq == 0) return;  // Forgetting to do this caused me a lot of annoyances.  Divide by zero danger below.

	if (Vd < 0) VdPos = -Vd;
	else VdPos = Vd;
	if (Vq < 0) VqPos = -Vq;
	else VqPos = Vq;

	if (VqPos < VdPos) {
		small = VqPos;
		big = VdPos;
	}
	else {
		small = VdPos;
		big = VqPos;
	}
	i = __builtin_divsd(((long)small) << 10, (int)big);   // small * 1024 / big gives an index from 0 to 1024 inclusive.
	fastDist = (unsigned int)((small + big - (small >> 1) - (small >> 2)) + (small >> 4));
	// fastDist must be in [0, 19687*2??]
	r = (__builtin_muluu((unsigned)distCorrection[i], (unsigned)fastDist) >> 15);  // scale down by 2^15 since distCorrection was scaled up by 2^15.
	rGlobal = r;

	rGlobal_filtered_times65536   	//= (15*rGlobal_filtered_times65536/65536 + 1*rGlobal) / 16 * 65536;
							 		//= (16*rGlobal_filtered_times65536/65536 - 1*rGlobal_filtered + 1*rGlobal) / 16 * 65536
									  = ((rGlobal_filtered_times65536 >> 12) - ((long)rGlobal_filtered) + ((long)rGlobal)) << 12;
	rGlobal_filtered = rGlobal_filtered_times65536 >> 16;

	if (r > R_MAX) {
		voltageDiskExceeded = 1;  // just a flag to let the rest of the program know that we just ran out of voltage.
		scale = __builtin_divud((unsigned long)R_MAX_TIMES_65536, (unsigned)r);
		small = (int)(__builtin_mulsu((int)small, (unsigned)scale) >> 16);
		big = (int)(__builtin_mulsu((int)big, (unsigned)scale) >> 16);
		IqRefNew = (int)(__builtin_mulsu((int)IqRef, (unsigned)scale) >> 16);
		IdRefNew = (int)(__builtin_mulsu((int)IdRef, (unsigned)scale) >> 16);  // what if I only did this to IdRef instead of both of them?  Then it would be field weakening.  I'll compare later.
		IqRef = IqRefNew + (__builtin_mulus(7,IqRef - IqRefNew) >> 3);
		IdRef = IdRefNew + (__builtin_mulus(7,IdRef - IdRefNew) >> 3);
	}
	else {
		voltageDiskExceeded = 0;
		return;
	}
	if (VqPos < VdPos) {
		if (Vq < 0) {
			Vq = -small;
		}
		else {
			Vq = small;
		}
		if (Vd < 0) {
			Vd = -big;
		}
		else {
			Vd = big;
		}
	}
	else {  // VdPos <= VqPos.  Now, Vd goes with small, and Vq goes with big.  watch the sign though.
		if (Vd < 0) {
			Vd = -small;
		}
		else {
			Vd = small;
		}
		if (Vq < 0) {
			Vq = -big;
		}
		else {
			Vq = big;
		}
	}
}

void SpaceVectorModulation() { 
	// faster way:
	if (Va <= Vb) {
		// 3 possibilities:
		// Va <= Vb <= Vc
		// Vc <= Va <= Vb
		// Va <= Vc <= Vb
		if (Va <= Vc) {  // Va is smallest.
			pdc1 = 0;
			pdc2 = Vb - Va;
			pdc3 = Vc - Va;
		}
		else { // Vc is smallest
			pdc1 = Va-Vc;
			pdc2 = Vb-Vc;
			pdc3 = 0;
		}
	}
	else {
		// 3 possibilities:
		// Vb <= Va <= Vc
		// Vb <= Vc <= Va
		// Vc <= Vb <= Va
		if (Vb <= Vc) { // Vb is smallest.
			pdc1 = Va-Vb;
			pdc2 = 0;
			pdc3 = Vc-Vb;
		}
		else {  // Vc is smallest.
			pdc1 = Va-Vc;
			pdc2 = Vb-Vc;
			pdc3 = 0;			
		}
	}

// Equivalent slower code below //////////////////
/*
	if (Vc <= Va && Vc <= Vb) {  // Vc is smallest
		// Q1, Q2
		pdc1 = Va-Vc;
		pdc2 = Vb-Vc;
		pdc3 = 0;
	}
	else if (Va <= Vb && Va <= Vc) { // Va is smallest 
		// Q3, Q4
		pdc1 = 0;
		pdc2 = Vb - Va;
		pdc3 = Vc - Va;
	}
	else { // Vb is smallest.
		// Q5, Q6
		pdc1 = Va-Vb;
		pdc2 = 0;
		pdc3 = Vc-Vb;
	}
*/
	if (pdc1 > MAX_DUTY) {
		pdc1 = MAX_DUTY;
	}
	if (pdc2 > MAX_DUTY) {
		pdc2 = MAX_DUTY;
	}
	if (pdc3 > MAX_DUTY) {
		pdc3 = MAX_DUTY;
	}
	// pdc1, 2, and 3 can never be negative because of above.  So, don't bother to test that.
#ifndef DEBUG_MODE
	PDC1 = pdc1;
	PDC2 = pdc2;
	PDC3 = pdc3;
#else
	PDC1 = 1000;
	PDC2 = 1000;
	PDC3 = 1000;
#endif
}

void ClearAllFaults() {
	ClearDesatFault();
	ClearFlipFlop();
}

void ClearFlipFlop() {
	O_LAT_CLEAR_FLIP_FLOP = 0;  // Really like 100nS would be enough.
	Delay1uS();
	O_LAT_CLEAR_FLIP_FLOP = 1;	
}

void ClearDesatFault() {	// reset must be pulled low for at least 1.2uS.
	static volatile int i = 0;
	O_LAT_CLEAR_DESAT = 0; 	// FOD8316 Datasheet says low for at least 1.2uS.  But then the stupid fault signal may not be cleared for 20 whole uS!
	Delay1uS(); Delay1uS(); Delay1uS();
	O_LAT_CLEAR_DESAT = 1;

	for (i = 0; i < 30; i++) {  // Now, let's waste more than 30uS waiting around for the desat fault signal to be cleared.
		Delay1uS();
	} // 30uS better be long enough!
}
void Delay1uS() {  // Assuming 30MIPs.
	Nop(); Nop(); Nop(); Nop();	Nop(); Nop(); Nop(); Nop();	Nop(); Nop(); Nop(); Nop();	Nop(); Nop(); Nop();
	Nop(); Nop(); Nop(); Nop();	Nop(); Nop(); Nop(); Nop();	Nop(); Nop(); Nop(); Nop();	Nop(); Nop(); Nop();
}

void InitTimers() {
	T1CON = 0;  // Make sure it starts out as 0.
	T1CONbits.TCKPS = 0b11;  // prescale of 256.  So, timer1 will run at 115200Hz if Fcy is 7.3728*4 MHz.
	PR1 = 0xFFFF;  // 
	T1CONbits.TON = 1; // Start the timer.

	T2CONbits.T32 = 1;  // 32 bit mode.
	T2CONbits.TCKPS = 0b11;  // 1:1 prescaler.
	T2CONbits.TCS = 0;  	// use internal 15MHz Fcy for the clock.
	PR3 = 0x0FFFF;		// HIGH 16 BITS.
	PR2 = 0x0FFFF;		// low 16 bits.
	// Now, TMR3:TMR2 makes up the 32 bit timer, running at 58.6KHz.

	T4CONbits.T32 = 1;  // 32 bit mode.
	T4CONbits.TCKPS = 0b00;  // 1:1 prescaler.
	T4CONbits.TCS = 0;  	// use internal 15MHz Fcy for the clock.
	PR5 = 0x0FFFF;		// HIGH 16 BITS.
	PR4 = 0x0FFFF;		// low 16 bits.

	T2CONbits.TON = 1;	// Start the timer.
	T4CONbits.TON = 1;	// Start the timer.

	TMR3 = 0;  	// Timer3:Timer2 high word
	TMR5 = 0;	// Timer5:Timer4 high word

	TMR2 = 0;  	// Timer3:Timer2 low word
	TMR4 = 0;	// Timer5:Timer4 low word
}	

// Assuming a 30MHz clock, one tick is 1/(58594*2) seconds.
void Delay(unsigned int time) {
	static volatile unsigned int temp;
	temp = TMR1;	
	while (TMR1 - temp < time) {
		ClrWdt();
	}
}
void DelaySeconds(int time) {
	static volatile int i;
	for (i = 0; i < time; i++) { 
		Delay(58594u);  // 0.5 second.
		Delay(58594u);  // 0.5 second.
	}
}
void DelayTenthsSecond(int time) {
	static volatile int i;
	for (i = 0; i < time; i++) { 
		Delay(5859*2);  // 58594*2 ticks in Delay is 1 second.  So, 1/10 of that.
	}
}

void InitCNModule() {
	CNEN1bits.CN0IE = 1;  // overcurrent fault.
	CNEN1bits.CN1IE = 1;  // desat fault.

	CNPU1bits.CN0PUE = 0; // Make sure internal pull-up is turned OFF on CN0.
	CNPU1bits.CN1PUE = 0; // Make sure internal pull-up is turned OFF on CN1.

	_CNIF = 0;  // Clear change notification interrupt flag just to make sure it starts cleared.
	_CNIP = 3;  // Set the priority level for interrupts to 3.
	_CNIE = 1;  // Make sure interrupts are enabled.
}

void __attribute__((__interrupt__, auto_psv)) _CNInterrupt(void) {
	IFS0bits.CNIF = 0;  // clear the interrupt flag.

	if (I_PORT_DESAT_FAULT == 0) {  // It just became 0 like 2Tcy's ago.
		faultBits |= DESAT_FAULT;
	}
	if (I_PORT_OVERCURRENT_FAULT == 0) {
		faultBits |= OVERCURRENT_FAULT;
	}
}

void InitIORegisters() {
	I_TRIS_THROTTLE = 1;		// 1 means configure as input.  A/D throttle.
	I_TRIS_CURRENT1	= 1;		// 1 means configure as input.  A/D current 1.
	I_TRIS_CURRENT2	= 1;		// 1 means configure as input.  A/D current 2.
	I_TRIS_INDEX	= 1;		// 1 means configure as input.  encoder index.  1 pulse per revolution.
	I_TRIS_QEA		= 1;		// 1 means configure as input.  encoder QEA
	I_TRIS_QEB		= 1;		// 1 means configure as input.  encoder QEB
	O_TRIS_CLEAR_FLIP_FLOP = 0;	// 0 means configure as output. clear flip flop.
	O_LAT_CLEAR_FLIP_FLOP = 1;  // bring LOW, then HIGH to clear the flip flop.

	_TRISF0 = 0;				// DEBUGGING PIN.  configure as output.
	_LATF0 = 0;					// initialize it as low.

	I_TRIS_TEMPERATURE = 1;	// 1 means configure as input.  A/D temperatureBasePlate.
	I_TRIS_REGEN_THROTTLE = 1;	// 1 means configure as input.  A/D regen throttle.
	I_TRIS_DESAT_FAULT = 1;		// 1 means configure as input.  desat fault.
	I_TRIS_OVERCURRENT_FAULT = 1;	// 1 means configure as input. overcurrent fault.
	I_TRIS_UNDERVOLTAGE_FAULT = 1;	// 1 means configure as input.  undervoltage fault.
	O_TRIS_LED = 0; 			// 0 means configure as output.  Status LED.
	O_LAT_LED = 1; 				 // high means turn ON the LED.
	O_TRIS_PRECHARGE_RELAY = 0;	// 0 means configure as output.  precharge relay control.
	O_LAT_PRECHARGE_RELAY = 0;	// HIGH means turn ON the precharge relay.
	I_TRIS_GLOBAL_FAULT	= 1;	
	O_TRIS_CONTACTOR = 0;		// 
	O_LAT_CONTACTOR = 0;	
	
	O_TRIS_CLEAR_DESAT = 0;	
	O_LAT_CLEAR_DESAT = 1;		// Bring low then high to clear desat.
	
	O_TRIS_PWM_3H = 0;		// 0 means configure as output.
	O_LAT_PWM_3H = 0;		// Low means OFF.
	
	O_TRIS_PWM_3L = 0;		// 0 means configure as output.
	O_LAT_PWM_3L = 0;		// Low means OFF.
			
	O_TRIS_PWM_2H = 0;		// 0 means configure as output.
	O_LAT_PWM_2H = 0;		// Low means OFF.
		
	O_TRIS_PWM_2L = 0;		// 0 means configure as output.
	O_LAT_PWM_2L = 0;		// Low means OFF.
	
	O_TRIS_PWM_1H = 0;		// 0 means configure as output.
	O_LAT_PWM_1H = 0;		// Low means OFF.
	
	O_TRIS_PWM_1L = 0;		// 0 means configure as output.
	O_LAT_PWM_1L = 0;		// Low means OFF.
	
	ADPCFG = 0b1111111001111000;  // 0 is analog.  1 is digital.
}

void TurnOffADAndPWM() {
	ADCON1bits.ADON = 0; // Pg. 416 in F.R.M.  Under "Enabling the Module".  Turn it off for a moment.
	PWMCON1 = 0;
	O_LAT_PWM_1H = 0;
	O_LAT_PWM_2H = 0;
	O_LAT_PWM_3H = 0;
	O_LAT_PWM_1L = 0;
	O_LAT_PWM_2L = 0;
	O_LAT_PWM_3L = 0;
	DTCON1 = 0;
	PTCON = 0;
	SEVTCMP = 0;
	ADCON1 = 0;
	ADCON2 = 0;
	ADCON3 = 0;
	ADCHS = 0;
}

void InitADAndPWM() {
    ADCON1bits.ADON = 0; // Pg. 416 in F.R.M.  Under "Enabling the Module".  Turn it off for a moment.

	// PWM Initialization

	PTPER = 1474;	// 29,491,200/((PTPER + 1)*2) = 9997Hz. 
	PDC1 = 0;
	PDC2 = 0;
	PDC3 = 0;

	PWMCON1 = 0b0000000000000000; 			// Pg. 339 in FRM.
	PWMCON1bits.PEN1H = 1; // enabled. Enables pwm1h.  That's not the correct procedure captain!  Put your hand on the key sir!  I'm sorry... I'm so sorry.. Put your hand on the key sir!  -War Games.
	PWMCON1bits.PEN1L = 1; // enabled. Enables pwmll.
	PWMCON1bits.PEN2H = 1; // enabled.
	PWMCON1bits.PEN2L = 1; // enabled.
	PWMCON1bits.PEN3H = 1; // enabled.
	PWMCON1bits.PEN3L = 1; // enabled. 
	
	PWMCON2 = 0;

    DTCON1 = 0;     		// Pg. 341 in Family Reference Manual. 
	DTCON1bits.DTAPS = 0b10;  // deadtime unit A prescale is 4*Tcy.
	DTCON1bits.DTA = 15;  	// 15 CLOCK periods.  That is 4*15*Tcy = 2uS of dead time assuming 30MHz.

//    FLTACON = 0b0000000010000001; // Pg. 343 in Family Reference Manual. The Fault A input pin functions in the cycle-by-cycle mode.

	PTCON = 0x8002;         // Pg. 337 in FRM.  Enable PWM for center aligned operation.
							// PTCON = 0b1000000000000010;  Pg. 337 in Family Reference Manual.
							// PTEN = 1; PWM time base is ON
							// PTSIDL = 0; PWM time base runs in CPU Idle mode
							// PTOPS = 0000; 1:1 Postscale
							// PTCKPS = 00; PWM time base input clock period is TCY (1:1 prescale)
							// PTMOD = 10; PWM time base operates in a continuous up/down counting mode

	// SEVTCMP: Special Event Compare Count Register 
    // Phase of ADC capture set relative to PWM cycle: 0 offset and counting up
    SEVTCMP = 2;        // Cannot be 0 -> turns off trigger (Missing from doc)
						// SEVTCMP = 0b0000000000000010;  Pg. 339 in Family Reference Manual.
						// SEVTDIR = 0; A special event trigger will occur when the PWM time base is counting upwards and...
						// SEVTCMP = 000000000000010; If SEVTCMP == PTMR<14:0>, then a special event trigger happens.


	// ============= ADC - Measure 
	// ADC setup for simultanous sampling
	// AN0 = CH1 = Throttle;
	// AN1 = CH2 = Current1;
	// AN2 = CH3 = Current2;
	// AN8 = CH0 = regen throttle;	// trade between these 2
	// AN7 = CH0 = Temperature;		// trade between these 2.

	ADCON1 = 0;  // Starts this way anyway.  But just to be sure.   

    ADCON1bits.FORM = 0;  // unsigned integer in the range 0-1023
    ADCON1bits.SSRC = 0b011;  // Motor Control PWM interval ends sampling and starts conversion

    // Simultaneous Sample Select bit (only applicable when CHPS = 01 or 1x)
    // Samples CH0, CH1, CH2, CH3 simultaneously (when CHPS = 1x)
    // Samples CH0 and CH1 simultaneously (when CHPS = 01)
    ADCON1bits.SIMSAM = 1; 
 
    // Sampling begins immediately after last conversion completes. 
    // SAMP bit is auto set.
    ADCON1bits.ASAM = 1;  

	ADCON2 = 0; // Pg. 407 in F.R.M.
    // Pg. 407 in F.R.M.
    // Samples CH0, CH1, CH2, CH3 simultaneously when CHPS = 1x
    ADCON2bits.CHPS = 0b10; // VCFG = 000; This selects the A/D High voltage as AVdd, and A/D Low voltage as AVss.
						 // SMPI = 0000; This makes an interrupt happen every time the A/D conversion process is done (for all 4 I guess, since they happen at the same time.)
						 // ALTS = 0; Always use MUX A input multiplexer settings
						 // BUFM = 0; Buffer configured as one 16-word buffer ADCBUF(15...0)


 	ADCON3 = 0; // Pg. 408 in F.R.M.
    // Pg. 408 in F.R.M.
    // A/D Conversion Clock Select bits = 16 * Tcy.  (31+1) * Tcy/2 = 16*Tcy.
	// The A/D conversion of 4 simultaneous conversions takes 4*12*A/D clock cycles.  The A/D clock is selected to be 4*Tcy.
    // So, it takes about 4*12*16*Tcy to complete 4 A/D conversions. That's 51.2uS. The pwm period is 100uS, since it's 10kHz.
	ADCON3bits.ADCS = 31;  // 16Tcy.


    // ADCHS: ADC Input Channel Select Register 
    ADCHS = 0; // Pg. 409 in F.R.M.

    // ADCHS: ADC Input Channel Select Register 
    // Pg. 409 in F.R.M.
    // CH0 positive input is AN7, temperatureBasePlate.
    ADCHSbits.CH0SA = 7;
    	
	// CH1 positive input is AN0, CH2 positive input is AN1, CH3 positive input is AN2.
	ADCHSbits.CH123SA = 0;

	// CH0 negative input is Vref-.
	ADCHSbits.CH0NA = 0;

	// CH1, CH2, CH3 negative inputs are Vref-, which is AVss, which is Vss.  haha.
    ADCHSbits.CH123NA = 0;

    // ADCSSL: ADC Input Scan Select Register 
    ADCSSL = 0; // Pg. 410 F.R.M.
				// I think it sets the order that the A/D inputs are done.  But I'm doing 4 all at the same time, so set it to 0?


    // Turn on A/D module
    ADCON1bits.ADON = 1; // Pg. 416 in F.R.M.  Under "Enabling the Module"
						 // ** It's important to set all the bits above before turning on the A/D module. **
						 // Now the A/D conversions start happening once ADON == 1.
	_ADIP = 4;			 // A/D interrupt priority set to 4.  Default is 4.
	IEC0bits.ADIE = 1;	 // Enable interrupts to happen when a A/D conversion is complete. Pg. 148 of F.R.M.  	

	PDC1 = 0;
	PDC2 = 0;
	PDC3 = 0;
}

void InitQEI() {
	// QEICON starts as all zeros.
	// The Quadrature Encoder Interface.  This is for enabling the encoder.
	switch (savedValues.motorType) {
		case AC_INDUCTION_MOTOR:
			QEICONbits.QEIM = 0b111; 	// enable QEI x4 mode with position counter reset by MAXCNT.  
			QEICONbits.PCDOUT = 1; 	 	// Position Counter Direction Status Output Enable (QEI logic controls state of I/O pin)
			QEICONbits.POSRES = 0;		// Position counter is not reset by index pulse. But there's no index pulse in my case.  haha. This is ignored in this situation.
			QEICONbits.SWPAB = 0;		// don't swap QEA and QEB inputs.
		
			DFLTCONbits.CEID = 1; 		// Interrupts due to position count errors disabled
			DFLTCONbits.QEOUT = 1; 		// Digital filter outputs enabled.  QEA, QEB. 0 means normal pin operation.
			DFLTCONbits.QECK = 0b011; 	// clock prescaler of 16. So, QEA or QEB must be high or low for a time of 16*3 Tcy's. 
										// Fcy = 30MHz.  3*16Tcy is the minimum pulse width required for QEA or QEB to change from high to low or low to high.
										// You can do at most 30,000,000/(3*16) = 625,000 of those pulses per second.  So, you can have at most
										// 625,000 * 4 clock counts per second, since resolution has been multiplied by 4 (QEA and QEB up and down transitions all cause a pulse to be seen by the microcontroller..)  
										//  That means the maximum detectable RPM is:
										// x rev/sec * 2096 clockCounts/Rev = 625,000 * 4 clockCounts/sec.  ASSUMING A 512 tick per revolution encoder like I"m using...
										// So, x = 1220 rev/sec.
										// 
			DFLTCONbits.IMV = 0b01;  	// INDEX pulse happens when QEA is high.  Irrelevant.  I'm not using the index pulse for the AC induction motor.
			MAXCNT = 0xFFFF; 	// reset the counter each time it reaches maxcnt.  It will reset anyway won't it?  I mean, after 0xFFFF is 0x0000!  haha.
								// Use this for the AC controller.  It's easier to measure speed of rotor with this setting, if there's no INDEX signal on the encoder.
			POSCNT = 0;  // How many ticks have gone by so far?  Starts out as zero anyway.  It's safe to write to it though.
		break;
		case NISSAN_LEAF_MOTOR:
			QEICONbits.QEIM = 0b100; 	// enable QEI x2 mode with position counter reset by INDEX pulse. This works out nicely, so that you get 512 ticks per index pulse, so it matches with the normalized 512 electrical degrees. 
			QEICONbits.PCDOUT = 1; 	 	// Position Counter Direction Status Output Enable (QEI logic controls state of I/O pin)
			QEICONbits.POSRES = 1;		// Position counter is reset by index pulse.	
			QEICONbits.SWPAB = 0;		// don't swap QEA and QEB inputs.  
	
			DFLTCONbits.CEID = 1; 		// Interrupts due to position count errors disabled
			DFLTCONbits.QEOUT = 1; 		// Digital filter outputs enabled.  QEA, QEB. 0 means normal pin operation.
			DFLTCONbits.QECK = 0b011; 	// clock prescaler of 16. So, QEA or QEB must be high or low for a time of 16*3 Tcy's. 
										// Fcy = 30MHz.  3*16Tcy is the minimum pulse width required for QEA or QEB to change from high to low or low to high.
										// You can do at most 30,000,000/(3*16) = ??? of those pulses per second.  So, you can have at most
										// ??? * 2 clock counts per second, since resolution has been multiplied by 2 (QEA and QEB up and down transitions all cause a pulse to be seen by the microcontroller..)  
										// 
			DFLTCONbits.IMV = 0b01;  	// INDEX pulse MUST happen when QEA is HIGH. It didn't work for QEA being low with the Leaf motor.
			MAXCNT = 511; 		// If going backwards, poscnt will go from 0 to maxcnt, ONLY IF it is caused by an index pulse!!!
								// Use this for the AC controller.  It's easier to measure speed of rotor with this setting, if there's no INDEX signal on the encoder.
			POSCNT = 15000;  	// It statts in an unknown state before the index pulse happens. Do this so I can know the index pulse hasn't happened yet.  It will vary from 14000 up to 16000 or whatever, and suddenly be forever trapped in 0,511 once the index pulse happens.
			myFirstEncoderIndexPulseTest.startTime = 0;
			myFirstEncoderIndexPulseTest.currentAngleOffset = 0;
			myFirstEncoderIndexPulseTest.testRunning = 1;
		break;
		case TOYOTA_MGR_MOTOR:
			QEICONbits.QEIM = 0b110; 	// enable QEI x4 mode with position counter reset by INDEX pulse. There are 2 electrical revolutions per index pulse with that motor type.  This works out nicely, so that you get 1024 ticks per 2 electrical revolutions per 1024 ticks, so 512 ticks per electrical revolution.
			QEICONbits.PCDOUT = 1; 	 	// Position Counter Direction Status Output Enable (QEI logic controls state of I/O pin)
			QEICONbits.POSRES = 1;		// Position counter is reset by index pulse.	
			QEICONbits.SWPAB = 0;		// don't swap QEA and QEB inputs.  
	
			DFLTCONbits.CEID = 1; 		// Interrupts due to position count errors disabled
			DFLTCONbits.QEOUT = 1; 		// Digital filter outputs enabled.  QEA, QEB. 0 means normal pin operation.
			DFLTCONbits.QECK = 0b011; 	// clock prescaler of 16. So, QEA or QEB must be high or low for a time of 16*3 Tcy's. 
										// Fcy = 30MHz.  3*16Tcy is the minimum pulse width required for QEA or QEB to change from high to low or low to high.
										// You can do at most 30,000,000/(3*16) = ??? of those pulses per second.  So, you can have at most
										// ??? * 2 clock counts per second, since resolution has been multiplied by 2 (QEA and QEB up and down transitions all cause a pulse to be seen by the microcontroller..)  
										// 
			DFLTCONbits.IMV = 0b01;  	// INDEX pulse MUST happen when QEA is HIGH and QEB is LOW. It didn't work for other combinations.
			MAXCNT = 1023; 		// If going backwards, poscnt will go from 0 to maxcnt, ONLY IF it is caused by an index pulse!!!
								// Use this for the AC controller.  It's easier to measure speed of rotor with this setting, if there's no INDEX signal on the encoder.
			POSCNT = 15000;  	// It statts in an unknown state before the index pulse happens. Do this so I can know the index pulse hasn't happened yet.  It will vary from 14000 up to 16000 or whatever, and suddenly be forever trapped in 0,511 once the index pulse happens.
			myFirstEncoderIndexPulseTest.startTime = 0;
			myFirstEncoderIndexPulseTest.currentAngleOffset = 0;
			myFirstEncoderIndexPulseTest.testRunning = 1;
		break;
		case PERMANENT_MAGNET_MOTOR_WITH_ENCODER:
			QEICONbits.QEIM = 0b110; 	// enable QEI x4 mode with position counter reset by INDEX pulse. There are 2 electrical revolutions per index pulse with that motor type.  This works out nicely, so that you get 1024 ticks per 2 electrical revolutions per 1024 ticks, so 512 ticks per electrical revolution.
			QEICONbits.PCDOUT = 1; 	 	// Position Counter Direction Status Output Enable (QEI logic controls state of I/O pin)
			QEICONbits.POSRES = 1;		// Position counter is reset by index pulse.	
			QEICONbits.SWPAB = 0;		// don't swap QEA and QEB inputs.  
	
			DFLTCONbits.CEID = 1; 		// Interrupts due to position count errors disabled
			DFLTCONbits.QEOUT = 1; 		// Digital filter outputs enabled.  QEA, QEB. 0 means normal pin operation.
			DFLTCONbits.QECK = 0b011; 	// clock prescaler of 16. So, QEA or QEB must be high or low for a time of 16*3 Tcy's. 
										// Fcy = 30MHz.  3*16Tcy is the minimum pulse width required for QEA or QEB to change from high to low or low to high.
										// You can do at most 30,000,000/(3*16) = ??? of those pulses per second.  So, you can have at most
										// ??? * 2 clock counts per second, since resolution has been multiplied by 2 (QEA and QEB up and down transitions all cause a pulse to be seen by the microcontroller..)  
										// 
			DFLTCONbits.IMV = 0b11;  	// INDEX pulse MUST happen when QEA & QEB is HIGH. It didn't work for QEA being low with the Leaf motor.
			MAXCNT = (savedValues2.encoderTicks << 2) - 1; 		// If going backwards, poscnt will go from 0 to maxcnt, ONLY IF it is caused by an index pulse!!!  Multiply by 4 since I"m doing 4* the resolution.
								// Use this for the AC controller.  It's easier to measure speed of rotor with this setting, if there's no INDEX signal on the encoder.
			POSCNT = 15000;  	// It statts in an unknown state before the index pulse happens. Do this so I can know the index pulse hasn't happened yet.  It will vary from 14000 up to 16000 or whatever, and suddenly be forever trapped in 0,511 once the index pulse happens.
			myFirstEncoderIndexPulseTest.startTime = 0;
			myFirstEncoderIndexPulseTest.currentAngleOffset = 0;
			myFirstEncoderIndexPulseTest.testRunning = 1;
		break;
	}
}

void EESaveValues() {  // save the new stuff.
	static volatile int i = 0;
	EEDataInRam1[0] = savedValues.motorType;
	EEDataInRam1[1] = savedValues.Kp;						// proportional gain shared between Id and Iq.
	EEDataInRam1[2] = savedValues.Ki;						// integreal gain shared between Id and Iq.
	EEDataInRam1[3] = savedValues.currentSensorAmpsPerVolt;	// Ex:  if a 
	EEDataInRam1[4] = savedValues.maxRegenPosition;			// 
	EEDataInRam1[5] = savedValues.minRegenPosition;			// 
	EEDataInRam1[6] = savedValues.minThrottlePosition;		//	
	EEDataInRam1[7] = savedValues.maxThrottlePosition;		// 
	EEDataInRam1[8] = savedValues.throttleFaultPosition;	// 
	EEDataInRam1[9] = savedValues.maxBatteryAmps;			//
	EEDataInRam1[10] = savedValues.maxBatteryAmpsRegen;		// 
	EEDataInRam1[11] = savedValues.maxMotorAmps;			//
	EEDataInRam1[12] = savedValues.maxMotorAmpsRegen;		// 
	EEDataInRam1[13] = savedValues.prechargeTime;			// 
	EEDataInRam1[14] = 0;
	EEDataInRam1[15] = 0;									// crc computed later.

	EEDataInRam2[0] = savedValues2.angleOffset;
	EEDataInRam2[1] = savedValues2.rotorTimeConstantIndex;
	EEDataInRam2[2] = savedValues2.numberOfPolePairs;
	EEDataInRam2[3] = savedValues2.maxRPM;
	EEDataInRam2[4] = savedValues2.throttleType;
	EEDataInRam2[5] = savedValues2.encoderTicks;
	EEDataInRam2[6] = savedValues2.dataToDisplaySet1;
	EEDataInRam2[7] = savedValues2.dataToDisplaySet2;
	EEDataInRam2[8] = savedValues2.KArrayIndex;
	EEDataInRam2[9] = 0;	// spares[0]
	EEDataInRam2[10] = 0;	// spares[1]
	EEDataInRam2[11] = 0;	// spares[2]
	EEDataInRam2[12] = 0;	// spares[3]
	EEDataInRam2[13] = 0;	// spares[4]
	EEDataInRam2[14] = 0;	// spares[5]
	EEDataInRam2[15] = 0;	// crc

	for (i = 0; i < 15; i++) {
		EEDataInRam1[15] += EEDataInRam1[i];	// compute checksum.
		EEDataInRam2[15] += EEDataInRam2[i];
	}

    _erase_eedata(EE_addr1, _EE_ROW);  // 
    _wait_eedata();  // #define _wait_eedata() { while (NVMCONbits.WR); }
	ClrWdt();
    _erase_eedata(EE_addr2, _EE_ROW);  // 
    _wait_eedata();  // #define _wait_eedata() { while (NVMCONbits.WR); }
	ClrWdt();
    _erase_eedata(EE_addr3, _EE_ROW);  // 
    _wait_eedata();  // #define _wait_eedata() { while (NVMCONbits.WR); }
	ClrWdt();
    _erase_eedata(EE_addr4, _EE_ROW);  // 
    _wait_eedata();  // #define _wait_eedata() { while (NVMCONbits.WR); }
	ClrWdt();
    // Write a row to EEPROM from array "EEDataInRam1"
    _write_eedata_row(EE_addr1, EEDataInRam1); // first copy of savedValues.
    _wait_eedata();  // 
	ClrWdt();
    // Write a row to EEPROM from array "EEDataInRam1".  
    _write_eedata_row(EE_addr2, EEDataInRam1); // 2nd copy of savedValues.
    _wait_eedata();  // 
	ClrWdt();
    // Write a row to EEPROM from array "EEDataInRam2"
    _write_eedata_row(EE_addr3, EEDataInRam2); // 1st copy of savedValues2.
    _wait_eedata();  // 
	ClrWdt();
    // Write a row to EEPROM from array "EEDataInRam2"
    _write_eedata_row(EE_addr4, EEDataInRam2); // 2nd copy of savedValues2.
    _wait_eedata();  // 
	ClrWdt();
	// 2 copies of the same thing, to be more robust.
}

void MoveDataFromEEPromToRAM() {
	static volatile int i = 0;
	static volatile unsigned int CRC1 = 0, CRC2 = 0, CRC3 = 0, CRC4 = 0;

	_memcpy_p2d16(EEDataInRam1, EE_addr1, _EE_ROW);
	_memcpy_p2d16(EEDataInRam2, EE_addr2, _EE_ROW);
	_memcpy_p2d16(EEDataInRam3, EE_addr3, _EE_ROW);
	_memcpy_p2d16(EEDataInRam4, EE_addr4, _EE_ROW);
	for (i = 0; i < 15; i++) { // Skip the last one, which is CRC. So, just go from 0 up to 14.
		CRC1 += EEDataInRam1[i];
		CRC2 += EEDataInRam2[i];
		CRC3 += EEDataInRam3[i];
		CRC4 += EEDataInRam4[i];		
	}

	if (EEDataInRam1[15] == CRC1) {  // crc from EEProm is OK for copy 1.  There has been a previously saved configuration.  
		savedValues.motorType = EEDataInRam1[0];
		savedValues.Kp = EEDataInRam1[1];		// 
		savedValues.Ki = EEDataInRam1[2];						// 
		savedValues.currentSensorAmpsPerVolt = EEDataInRam1[3];		// 
		savedValues.maxRegenPosition = EEDataInRam1[4];		// 
		savedValues.minRegenPosition = EEDataInRam1[5];		// 
		savedValues.minThrottlePosition = EEDataInRam1[6];	// 
		savedValues.maxThrottlePosition = EEDataInRam1[7];	// 
		savedValues.throttleFaultPosition = EEDataInRam1[8];	// 
		savedValues.maxBatteryAmps = EEDataInRam1[9];		// 
		savedValues.maxBatteryAmpsRegen = EEDataInRam1[10];	// 
		savedValues.maxMotorAmps = EEDataInRam1[11];		// 
		savedValues.maxMotorAmpsRegen = EEDataInRam1[12];	//  
		savedValues.prechargeTime = EEDataInRam1[13];		//
		savedValues.spares[0] = EEDataInRam1[14];			//
		savedValues.crc = EEDataInRam1[15];					// 
	}
	else if (EEDataInRam2[15] == CRC2) {  // crc from EEProm is OK for copy 1.  There has been a previously saved configuration.  
		savedValues.motorType = EEDataInRam2[0];
		savedValues.Kp = EEDataInRam2[1];		// 
		savedValues.Ki = EEDataInRam2[2];						// 
		savedValues.currentSensorAmpsPerVolt = EEDataInRam2[3];		// 
		savedValues.maxRegenPosition = EEDataInRam2[4];		// 
		savedValues.minRegenPosition = EEDataInRam2[5];		// 
		savedValues.minThrottlePosition = EEDataInRam2[6];	// 
		savedValues.maxThrottlePosition = EEDataInRam2[7];	// 
		savedValues.throttleFaultPosition = EEDataInRam2[8];	// 
		savedValues.maxBatteryAmps = EEDataInRam2[9];		// 
		savedValues.maxBatteryAmpsRegen = EEDataInRam2[10];	// 
		savedValues.maxMotorAmps = EEDataInRam2[11];		// 
		savedValues.maxMotorAmpsRegen = EEDataInRam2[12];	//  
		savedValues.prechargeTime = EEDataInRam2[13];		//
		savedValues.spares[0] = EEDataInRam2[14];			//
		savedValues.crc = EEDataInRam2[15];					// 
	}
	else {
		savedValues = savedValuesDefault;
		savedValues.crc = 	savedValues.motorType + 
							savedValues.Kp + 
							savedValues.Ki + 
							savedValues.currentSensorAmpsPerVolt + 
							savedValues.maxRegenPosition + 
							savedValues.minRegenPosition + 
							savedValues.minThrottlePosition + 
							savedValues.maxThrottlePosition + 
							savedValues.throttleFaultPosition + 
							savedValues.maxBatteryAmps + 
							savedValues.maxBatteryAmpsRegen + 
							savedValues.maxMotorAmps + 
							savedValues.maxMotorAmpsRegen + 
							savedValues.prechargeTime + 
							savedValues.spares[0];
	}
	if (EEDataInRam3[15] == CRC3) {
		savedValues2.angleOffset = EEDataInRam3[0];
		myAngleOffsetTest.currentAngleOffset = savedValues2.angleOffset;	// this is the working copy
		savedValues2.rotorTimeConstantIndex = EEDataInRam3[1];		// 
		myRotorTest.timeConstantIndex = savedValues2.rotorTimeConstantIndex;	// this is the working copy
		savedValues2.numberOfPolePairs = EEDataInRam3[2];						// 
		savedValues2.maxRPM = EEDataInRam3[3];		// 
		savedValues2.throttleType = EEDataInRam3[4];
		savedValues2.encoderTicks = EEDataInRam3[5];
		savedValues2.dataToDisplaySet1 = EEDataInRam3[6];
		savedValues2.dataToDisplaySet2 = EEDataInRam3[7];
		savedValues2.KArrayIndex = EEDataInRam3[8];
		myMotorSaliencyTest.KArrayIndex = savedValues2.KArrayIndex;	// this is the working copy

		savedValues2.spares[0] = EEDataInRam3[9];
		savedValues2.spares[1] = EEDataInRam3[10];
		savedValues2.spares[2] = EEDataInRam3[11];
		savedValues2.spares[3] = EEDataInRam3[12];
		savedValues2.spares[4] = EEDataInRam3[13];
		savedValues2.spares[5] = EEDataInRam3[14];
		savedValues2.crc = EEDataInRam3[15];					// 
	}
	else if (EEDataInRam4[15] == CRC4) {
		savedValues2.angleOffset = EEDataInRam4[0];
		myAngleOffsetTest.currentAngleOffset = savedValues2.angleOffset;	// this is the working copy
		savedValues2.rotorTimeConstantIndex = EEDataInRam4[1];		// 
		myRotorTest.timeConstantIndex = savedValues2.rotorTimeConstantIndex;	// this is the working copy
		savedValues2.numberOfPolePairs = EEDataInRam4[2];						// 
		savedValues2.maxRPM = EEDataInRam4[3];		// 
		savedValues2.throttleType = EEDataInRam4[4];
		savedValues2.encoderTicks = EEDataInRam4[5];
		savedValues2.dataToDisplaySet1 = EEDataInRam4[6];
		savedValues2.dataToDisplaySet2 = EEDataInRam4[7];
		savedValues2.KArrayIndex = EEDataInRam3[8];
		myMotorSaliencyTest.KArrayIndex = savedValues2.KArrayIndex;	// this is the working copy

		savedValues2.spares[0] = EEDataInRam4[9];
		savedValues2.spares[1] = EEDataInRam4[10];
		savedValues2.spares[2] = EEDataInRam4[11];
		savedValues2.spares[3] = EEDataInRam4[12];
		savedValues2.spares[4] = EEDataInRam4[13];
		savedValues2.spares[5] = EEDataInRam4[14];
		savedValues2.crc = EEDataInRam4[15];					// 
	}
	else {	// There wasn't a single good copy.  Load the default configuration.
		savedValues2 = savedValuesDefault2;
		myAngleOffsetTest.currentAngleOffset = savedValues2.angleOffset;	// this is the working copy
		myRotorTest.timeConstantIndex = savedValues2.rotorTimeConstantIndex;	// this is the working copy
		myMotorSaliencyTest.KArrayIndex = savedValues2.KArrayIndex;	// this is the working copy

		savedValues2.crc =  savedValues2.angleOffset + 
							savedValues2.rotorTimeConstantIndex + 
							savedValues2.numberOfPolePairs + 
							savedValues2.maxRPM + 
							savedValues2.throttleType +
							savedValues2.encoderTicks +
							savedValues2.dataToDisplaySet1 + 
							savedValues2.dataToDisplaySet2 +
							savedValues2.KArrayIndex +
							savedValues2.spares[0] + 
							savedValues2.spares[1] +
							savedValues2.spares[2] + 
							savedValues2.spares[3] +
							savedValues2.spares[4] + 
							savedValues2.spares[5];
	}
}

void InitializeThrottleAndCurrentVariables() {
	// run this at startup, when there's a new savedValues.currentSensorAmpsPerVolt
	// Ex:  MaxMotorAmps = 300
	// 		currentSensorAmpsPerVolt = 480, which is for a LEM Hass 300-s
	// 		ticksPerAmp_times128 = 26214/480 = 54.6 = 54
	//		totalADTicks_times128 = 54 * maxMotorAmps = 54 * 300 = 16200
	//		maxMotorCurrentNormalized = 16200 >> 3 = 2025
	//
	//
	//
	//
	//
	// Ex:  MaxMotorAmps = 8
	// 		currentSensorAmpsPerVolt = 16, which is for a lem hass 50-s with 
	//		ticksPerAmp_times128 = 27214/16 = 1700.8 = 1700
	// 		totalADTicks_times128 = 1700 * maxMotorAmps = 1700 * 8 = 13600
	//		maxMotorCurrentNormalized = 1700
	//
	//
	//
	//
	//
	static volatile int ADTicksPerAmp_times128 = 0;
	static volatile long totalADTicks_times128 = 0;

	//ADTicksPerAmp_times128 = 128 * (1024/5) / currentSensorAmpsPerVolt; // 1024 A/D ticks per 5v.
	//ADTicksPerAmp_times128 = 26214 / currentSensorAmpsPerVolt;
	ADTicksPerAmp_times128 = (int)__builtin_divsd(26214L,(int)savedValues.currentSensorAmpsPerVolt);
//	ampToNormalizedMultiplier = ADTicksPerAmp_times128 >> 5;
	//totalADTicks_times128 = (ADTicksPerAmp_times128 * savedValues.maxMotorRegenAmps);
	totalADTicks_times128 = __builtin_mulss((int)ADTicksPerAmp_times128, (int)savedValues.maxMotorAmpsRegen);
	if ((totalADTicks_times128 >> 3) > 4096) {
		maxMotorCurrentNormalizedRegen = 4096;
	}
	else {
		maxMotorCurrentNormalizedRegen = totalADTicks_times128 >> 3;  // to go from [-256*128, 256*128] to [-4096, 4096], divide by 8.  I'm assuming a MAX current sensor acceptable range from [1.25v, 3.75v].
	}

	//totalADTicksRegen_times128 = (ADTicksPerAmp_times128 * savedValues.maxMotorRegenAmps);
	totalADTicks_times128 = __builtin_mulss((int)ADTicksPerAmp_times128, (int)savedValues.maxMotorAmps);
	if ((totalADTicks_times128 >> 3) > 4096) {
		maxMotorCurrentNormalized = 4096;
	}
	else {
		maxMotorCurrentNormalized = totalADTicks_times128 >> 3;  // to go from [-256*128, 256*128] to [-4096, 4096], divide by 8.  I'm assuming a current sensor acceptable range from [1.25v, 3.75v].
	}

	totalADTicks_times128 = __builtin_mulss((int)ADTicksPerAmp_times128, (int)savedValues.maxBatteryAmps);	; // go from amps to normalized.  Normalized is following the above process.
	if ((totalADTicks_times128 >> 3) > 4096) {
		maxBatteryCurrentNormalized = 4096;
	}
	else {
		maxBatteryCurrentNormalized = totalADTicks_times128 >> 3;  // to go from [-256*128, 256*128] to [-4096, 4096], divide by 8.  I'm assuming a current sensor acceptable range from [1.25v, 3.75v].
	}

	totalADTicks_times128 = __builtin_mulss((int)ADTicksPerAmp_times128, (int)savedValues.maxBatteryAmpsRegen);	; // go from amps to normalized.  Normalized is following the above process.
	if ((totalADTicks_times128 >> 3) > 4096) {
		maxBatteryCurrentNormalizedRegen = 4096;
	}
	else {
		maxBatteryCurrentNormalizedRegen = totalADTicks_times128 >> 3;  // to go from [-256*128, 256*128] to [-4096, 4096], divide by 8.  I'm assuming a current sensor acceptable range from [1.25v, 3.75v].
	}
	currentSensorAmpsPerVoltTimes5 = (int)__builtin_mulss((int)savedValues.currentSensorAmpsPerVolt, 5);  // when converting from the normalized [0, 4096] to real life amps, the conversion factor is currentSensorAmpsPerVolt*5/16384.
}

void GrabDataSnapshot() {
	static volatile int delta = 0;
	static volatile int offset = 0;
	static volatile int i = 0;

	myDataStream.Id_times10 = Id;
	myDataStream.Iq_times10 = Iq;
	myDataStream.IdRef_times10 = IdRef;
	myDataStream.IqRef_times10 = IqRef;
	myDataStream.Vd = Vd;
	myDataStream.Vq = Vq;
	myDataStream.Ia_times10 = Ia;
	myDataStream.Ib_times10 = Ib;
	myDataStream.Ic_times10 = Ic;
	myDataStream.Va = Va;
	myDataStream.Vb = Vb;
	myDataStream.Vc = Vc;
	myDataStream.rawThrottle = rawThrottle;
	myDataStream.throttle = throttle;
	myDataStream.temperature = temperatureBasePlate;
	myDataStream.slipSpeedRPM = slipSpeedRPS_times16;
	myDataStream.electricalSpeedRPM = rotorFluxRPS_times16; // revolutions per 16 seconds for the rotor flux.
	myDataStream.mechanicalSpeedRPM = RPS_times16; // REVOLUTIONS PER 16 SECONDS.
	myDataStream.batteryAmps_times10 = batteryCurrentNormalized; // in the range of [-4096, 4096], where 4096 corresponds to 3.75v on the AD feedback, and -4096 corresponds to 1.25v.  Later convert that to amps.


	myDataStream.slipSpeedRPM = __builtin_mulss(15,myDataStream.slipSpeedRPM) >> 2;
	myDataStream.mechanicalSpeedRPM = __builtin_mulss(15,myDataStream.mechanicalSpeedRPM) >> 2;
	myDataStream.electricalSpeedRPM = __builtin_mulss(15,myDataStream.electricalSpeedRPM) >> 2;
	myDataStream.batteryAmps_times10 = __builtin_mulss(myDataStream.batteryAmps_times10*5, currentSensorAmpsPerVoltTimes5) >> 13;  // [-4096, 4096] -> [-amps*10, amps*10].  To go from 4096 to amps*10, do 10 * x ticks * currentSensorAmpsPerVolt * 1.25 volts / 4096 ticks.
	myDataStream.Ia_times10 = __builtin_mulss(myDataStream.Ia_times10*5, currentSensorAmpsPerVoltTimes5) >> 13;  // [-4096, 4096] -> [-amps*10, amps*10].  To go from 4096 to amps*10, do 10 * x ticks * currentSensorAmpsPerVolt * 1.25 volts / 4096 ticks.
	myDataStream.Ib_times10 = __builtin_mulss(myDataStream.Ib_times10*5, currentSensorAmpsPerVoltTimes5) >> 13;  // [-4096, 4096] -> [-amps*10, amps*10].  To go from 4096 to amps*10, do 10 * x ticks * currentSensorAmpsPerVolt * 1.25 volts / 4096 ticks.
	myDataStream.Ic_times10 = __builtin_mulss(myDataStream.Ic_times10*5, currentSensorAmpsPerVoltTimes5) >> 13;  // [-4096, 4096] -> [-amps*10, amps*10].  To go from 4096 to amps*10, do 10 * x ticks * currentSensorAmpsPerVolt * 1.25 volts / 4096 ticks.
	myDataStream.IdRef_times10 = __builtin_mulss(myDataStream.IdRef_times10*5, currentSensorAmpsPerVoltTimes5) >> 13;  // [-4096, 4096] -> [-amps*10, amps*10].  To go from 4096 to amps*10, do 10 * x ticks * currentSensorAmpsPerVolt * 1.25 volts / 4096 ticks.
	myDataStream.IqRef_times10 = __builtin_mulss(myDataStream.IqRef_times10*5, currentSensorAmpsPerVoltTimes5) >> 13;  // [-4096, 4096] -> [-amps*10, amps*10].  To go from 4096 to amps*10, do 10 * x ticks * currentSensorAmpsPerVolt * 1.25 volts / 4096 ticks.
	myDataStream.Id_times10 = __builtin_mulss(myDataStream.Id_times10*5, currentSensorAmpsPerVoltTimes5) >> 13;  // [-4096, 4096] -> [-amps*10, amps*10].  To go from 4096 to amps*10, do 10 * x ticks * currentSensorAmpsPerVolt * 1.25 volts / 4096 ticks.
	myDataStream.Iq_times10 = __builtin_mulss(myDataStream.Iq_times10*5, currentSensorAmpsPerVoltTimes5) >> 13;  // [-4096, 4096] -> [-amps*10, amps*10].  To go from 4096 to amps*10, do 10 * x ticks * currentSensorAmpsPerVolt * 1.25 volts / 4096 ticks.
// How to convert from Va to real volts?  I think just divide by 1690 and multiply by batterypackvoltage?  1690 is the max radius.
//	myDataStream.Vd = __builtin_divsd(__builtin_mulss(myDataStream.Vd, savedValues2.packVoltage), R_MAX);
//	myDataStream.Vq = __builtin_divsd(__builtin_mulss(myDataStream.Vq, savedValues2.packVoltage), R_MAX);
//	myDataStream.Va = __builtin_divsd(__builtin_mulss(myDataStream.Va, savedValues2.packVoltage), R_MAX);
//	myDataStream.Vb = __builtin_divsd(__builtin_mulss(myDataStream.Vb, savedValues2.packVoltage), R_MAX);
//	myDataStream.Vc = __builtin_divsd(__builtin_mulss(myDataStream.Vc, savedValues2.packVoltage), R_MAX);
	myDataStream.percentOfVoltageDiskBeingUsed = __builtin_divsd(__builtin_mulss(100,rGlobal_filtered), R_MAX);
	myDataStream.temperature = __builtin_divsd(4812800L,myDataStream.temperature) - 5700;  // convert to the variable resisance value. 16077 
	delta = 32;
	offset = 64;
	for (i = 0; i < 6; i++) {
		if (myDataStream.temperature >= celciusToResistance[offset-1]) {
			offset = offset - delta;
		}
		else if (myDataStream.temperature < celciusToResistance[offset-1]) {
			offset = offset + delta;
		}
		delta >>= 1;
	}
	myDataStream.temperature = offset;
	myDataStream.timer = counter1k - myDataStream.startTime;
}

//RCON
void __attribute__ ((__interrupt__,auto_psv)) _MathError(void) {
	while (1);
}
void __attribute__ ((__interrupt__,auto_psv)) _StackError(void) {
	while (1);
}
void __attribute__ ((__interrupt__,auto_psv)) _AddressError(void) {
	while (1);
}
void __attribute__ ((__interrupt__,auto_psv)) _OscillatorFail(void) {
	while (1);
}
