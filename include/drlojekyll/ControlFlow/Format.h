// Copyright 2019, Trail of Bits. All rights reserved.

#pragma once

#include <drlojekyll/ControlFlow/Program.h>

namespace hyde {

class OutputStream;

OutputStream &operator<<(OutputStream &os, DataColumn col);
OutputStream &operator<<(OutputStream &os, DataIndex index);
OutputStream &operator<<(OutputStream &os, DataTable table);
OutputStream &operator<<(OutputStream &os, DataVector vec);
OutputStream &operator<<(OutputStream &os, DataVariable var);
OutputStream &operator<<(OutputStream &os, ProgramPublishRegion region);
OutputStream &operator<<(OutputStream &os, ProgramCallRegion region);
OutputStream &operator<<(OutputStream &os, ProgramReturnRegion region);
OutputStream &operator<<(OutputStream &os, ProgramTupleCompareRegion region);
OutputStream &operator<<(OutputStream &os, ProgramTestAndSetRegion region);
OutputStream &operator<<(OutputStream &os, ProgramGenerateRegion region);
OutputStream &operator<<(OutputStream &os, ProgramLetBindingRegion region);
OutputStream &operator<<(OutputStream &os, ProgramWorkerIdRegion region);
OutputStream &operator<<(OutputStream &os, ProgramVectorLoopRegion region);
OutputStream &operator<<(OutputStream &os, ProgramVectorAppendRegion region);
OutputStream &operator<<(OutputStream &os, ProgramVectorClearRegion region);
OutputStream &operator<<(OutputStream &os, ProgramVectorSwapRegion region);
OutputStream &operator<<(OutputStream &os, ProgramVectorUniqueRegion region);
OutputStream &operator<<(OutputStream &os, ProgramUpdateCountRegion region);
OutputStream &operator<<(OutputStream &os, ProgramChangeRecordRegion region);
OutputStream &operator<<(OutputStream &os, ProgramCheckMemberRegion region);
OutputStream &operator<<(OutputStream &os, ProgramCheckRecordRegion region);
OutputStream &operator<<(OutputStream &os, ProgramCommitSweepRegion region);
OutputStream &operator<<(OutputStream &os, ProgramGroupUpdateRegion region);
OutputStream &operator<<(OutputStream &os, ProgramClaimRegion region);
OutputStream &operator<<(OutputStream &os, ProgramRetireRegion region);
OutputStream &operator<<(OutputStream &os, ProgramNetBatchRegion region);
OutputStream &operator<<(OutputStream &os, ProgramTableJoinRegion region);
OutputStream &operator<<(OutputStream &os, ProgramTableProductRegion region);
OutputStream &operator<<(OutputStream &os, ProgramTableScanRegion region);
OutputStream &operator<<(OutputStream &os, ProgramInductionRegion region);
OutputStream &operator<<(OutputStream &os, ProgramSeriesRegion region);
OutputStream &operator<<(OutputStream &os, ProgramParallelRegion region);
OutputStream &operator<<(OutputStream &os, ProgramRegion region);
OutputStream &operator<<(OutputStream &os, ProgramProcedure proc);
OutputStream &operator<<(OutputStream &os, Program program);

}  // namespace hyde
