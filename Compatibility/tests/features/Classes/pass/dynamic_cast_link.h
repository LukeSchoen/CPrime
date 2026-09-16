#pragma once
struct RttiBase { virtual int value() const { return 1; } };
struct RttiDerived : RttiBase { int value() const override { return 2; } };
RttiBase *make_rtti_object(bool derived);
