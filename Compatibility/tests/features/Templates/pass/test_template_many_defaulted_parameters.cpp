// EXPECT_COMPILE_ONLY: 1
/* boost::mp11's append_111_impl declares one hundred and ten defaulted class
   parameters; a fixed thirty-two entry parameter table rejected it with
   "too many template parameters".  The parameter kind masks must keep
   tracking the kind of every parameter, so the default of each parameter
   here names an earlier one: a mask that lost the high bits would bind the
   wrong kind and reject the alias. */
template<
    class T00 = int,  class T01 = T00, class T02 = T01, class T03 = T02, class T04 = T03,
    class T05 = T04,  class T06 = T05, class T07 = T06, class T08 = T07, class T09 = T08,
    class T10 = T09,  class T11 = T10, class T12 = T11, class T13 = T12, class T14 = T13,
    class T15 = T14,  class T16 = T15, class T17 = T16, class T18 = T17, class T19 = T18,
    class T20 = T19,  class T21 = T20, class T22 = T21, class T23 = T22, class T24 = T23,
    class T25 = T24,  class T26 = T25, class T27 = T26, class T28 = T27, class T29 = T28,
    class T30 = T29,  class T31 = T30, class T32 = T31, class T33 = T32, class T34 = T33,
    class T35 = T34,  class T36 = T35, class T37 = T36, class T38 = T37, class T39 = T38,
    class T40 = T39,  class T41 = T40, class T42 = T41, class T43 = T42, class T44 = T43,
    class T45 = T44,  class T46 = T45, class T47 = T46, class T48 = T47, class T49 = T48,
    class T50 = T49,  class T51 = T50, class T52 = T51, class T53 = T52, class T54 = T53,
    class T55 = T54,  class T56 = T55, class T57 = T56, class T58 = T57, class T59 = T58,
    class T60 = T59,  class T61 = T60, class T62 = T61, class T63 = T62, class T64 = T63,
    class T65 = T64,  class T66 = T65, class T67 = T66, class T68 = T67, class T69 = T68,
    class T70 = T69,  class T71 = T70, class T72 = T71, class T73 = T72, class T74 = T73,
    class T75 = T74,  class T76 = T75, class T77 = T76, class T78 = T77, class T79 = T78,
    class T80 = T79,  class T81 = T80, class T82 = T81, class T83 = T82, class T84 = T83,
    class T85 = T84,  class T86 = T85, class T87 = T86, class T88 = T87, class T89 = T88,
    class T90 = T89,  class T91 = T90, class T92 = T91, class T93 = T92, class T94 = T93,
    class T95 = T94,  class T96 = T95, class T97 = T96, class T98 = T97, class T99 = T98,
    class TA0 = T99, class TA1 = TA0, class TA2 = TA1, class TA3 = TA2, class TA4 = TA3,
    class TA5 = TA4, class TA6 = TA5, class TA7 = TA6, class TA8 = TA7, class TA9 = TA8>
struct ManyDefaults
{
  typedef TA9 type;
};

static_assert(sizeof(ManyDefaults<>::type) == sizeof(int), "all parameters default");
