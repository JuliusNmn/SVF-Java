; ModuleID = 'example.c'
source_filename = "example.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [6 x i8] c"%c %c\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @swap(i8** noundef %p, i8** noundef %q) #0 !dbg !10 {
entry:
  %p.addr = alloca i8**, align 8
  %q.addr = alloca i8**, align 8
  %t = alloca i8*, align 8
  store i8** %p, i8*** %p.addr, align 8
  call void @llvm.dbg.declare(metadata i8*** %p.addr, metadata !17, metadata !DIExpression()), !dbg !18
  store i8** %q, i8*** %q.addr, align 8
  call void @llvm.dbg.declare(metadata i8*** %q.addr, metadata !19, metadata !DIExpression()), !dbg !20
  call void @llvm.dbg.declare(metadata i8** %t, metadata !21, metadata !DIExpression()), !dbg !22
  %0 = load i8**, i8*** %p.addr, align 8, !dbg !23
  %1 = load i8*, i8** %0, align 8, !dbg !24
  store i8* %1, i8** %t, align 8, !dbg !22
  %2 = load i8**, i8*** %q.addr, align 8, !dbg !25
  %3 = load i8*, i8** %2, align 8, !dbg !26
  %4 = load i8**, i8*** %p.addr, align 8, !dbg !27
  store i8* %3, i8** %4, align 8, !dbg !28
  %5 = load i8*, i8** %t, align 8, !dbg !29
  %6 = load i8**, i8*** %q.addr, align 8, !dbg !30
  store i8* %5, i8** %6, align 8, !dbg !31
  ret void, !dbg !32
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @print(i8* noundef %a, i8* noundef %b) #0 !dbg !33 {
entry:
  %a.addr = alloca i8*, align 8
  %b.addr = alloca i8*, align 8
  store i8* %a, i8** %a.addr, align 8
  call void @llvm.dbg.declare(metadata i8** %a.addr, metadata !36, metadata !DIExpression()), !dbg !37
  store i8* %b, i8** %b.addr, align 8
  call void @llvm.dbg.declare(metadata i8** %b.addr, metadata !38, metadata !DIExpression()), !dbg !39
  %0 = load i8*, i8** %a.addr, align 8, !dbg !40
  %1 = load i8, i8* %0, align 1, !dbg !41
  %conv = sext i8 %1 to i32, !dbg !41
  %2 = load i8*, i8** %b.addr, align 8, !dbg !42
  %3 = load i8, i8* %2, align 1, !dbg !43
  %conv1 = sext i8 %3 to i32, !dbg !43
  %call = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([6 x i8], [6 x i8]* @.str, i64 0, i64 0), i32 noundef %conv, i32 noundef %conv1), !dbg !44
  ret void, !dbg !45
}

declare dso_local i32 @printf(i8* noundef, ...) #2

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !46 {
entry:
  %retval = alloca i32, align 4
  %a1 = alloca i8, align 1
  %b1 = alloca i8, align 1
  %a = alloca i8*, align 8
  %b = alloca i8*, align 8
  store i32 0, i32* %retval, align 4
  call void @llvm.dbg.declare(metadata i8* %a1, metadata !48, metadata !DIExpression()), !dbg !49
  store i8 107, i8* %a1, align 1, !dbg !49
  call void @llvm.dbg.declare(metadata i8* %b1, metadata !50, metadata !DIExpression()), !dbg !51
  store i8 108, i8* %b1, align 1, !dbg !51
  call void @llvm.dbg.declare(metadata i8** %a, metadata !52, metadata !DIExpression()), !dbg !53
  store i8* %a1, i8** %a, align 8, !dbg !53
  call void @llvm.dbg.declare(metadata i8** %b, metadata !54, metadata !DIExpression()), !dbg !55
  store i8* %b1, i8** %b, align 8, !dbg !55
  call void @swap(i8** noundef %a, i8** noundef %b), !dbg !56
  %0 = load i8*, i8** %a, align 8, !dbg !57
  %1 = load i8*, i8** %b, align 8, !dbg !58
  call void @print(i8* noundef %0, i8* noundef %1), !dbg !59
  %2 = load i8*, i8** %a, align 8, !dbg !60
  %3 = ptrtoint i8* %2 to i32, !dbg !61
  ret i32 %3, !dbg !62
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 14.0.0", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "example.c", directory: "/home/julius/SVF-Java", checksumkind: CSK_MD5, checksum: "8eafc23ad28577867207187aed631a75")
!2 = !{!3}
!3 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!4 = !{i32 7, !"Dwarf Version", i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = !{i32 1, !"wchar_size", i32 4}
!7 = !{i32 7, !"uwtable", i32 1}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{!"clang version 14.0.0"}
!10 = distinct !DISubprogram(name: "swap", scope: !1, file: !1, line: 1, type: !11, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !16)
!11 = !DISubroutineType(types: !12)
!12 = !{null, !13, !13}
!13 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !14, size: 64)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!16 = !{}
!17 = !DILocalVariable(name: "p", arg: 1, scope: !10, file: !1, line: 1, type: !13)
!18 = !DILocation(line: 1, column: 18, scope: !10)
!19 = !DILocalVariable(name: "q", arg: 2, scope: !10, file: !1, line: 1, type: !13)
!20 = !DILocation(line: 1, column: 28, scope: !10)
!21 = !DILocalVariable(name: "t", scope: !10, file: !1, line: 2, type: !14)
!22 = !DILocation(line: 2, column: 9, scope: !10)
!23 = !DILocation(line: 2, column: 14, scope: !10)
!24 = !DILocation(line: 2, column: 13, scope: !10)
!25 = !DILocation(line: 3, column: 14, scope: !10)
!26 = !DILocation(line: 3, column: 13, scope: !10)
!27 = !DILocation(line: 3, column: 9, scope: !10)
!28 = !DILocation(line: 3, column: 11, scope: !10)
!29 = !DILocation(line: 4, column: 13, scope: !10)
!30 = !DILocation(line: 4, column: 9, scope: !10)
!31 = !DILocation(line: 4, column: 11, scope: !10)
!32 = !DILocation(line: 5, column: 1, scope: !10)
!33 = distinct !DISubprogram(name: "print", scope: !1, file: !1, line: 6, type: !34, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !16)
!34 = !DISubroutineType(types: !35)
!35 = !{null, !14, !14}
!36 = !DILocalVariable(name: "a", arg: 1, scope: !33, file: !1, line: 6, type: !14)
!37 = !DILocation(line: 6, column: 18, scope: !33)
!38 = !DILocalVariable(name: "b", arg: 2, scope: !33, file: !1, line: 6, type: !14)
!39 = !DILocation(line: 6, column: 27, scope: !33)
!40 = !DILocation(line: 7, column: 24, scope: !33)
!41 = !DILocation(line: 7, column: 23, scope: !33)
!42 = !DILocation(line: 7, column: 28, scope: !33)
!43 = !DILocation(line: 7, column: 27, scope: !33)
!44 = !DILocation(line: 7, column: 7, scope: !33)
!45 = !DILocation(line: 8, column: 1, scope: !33)
!46 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 9, type: !47, scopeLine: 9, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !16)
!47 = !DISubroutineType(types: !2)
!48 = !DILocalVariable(name: "a1", scope: !46, file: !1, line: 10, type: !15)
!49 = !DILocation(line: 10, column: 12, scope: !46)
!50 = !DILocalVariable(name: "b1", scope: !46, file: !1, line: 11, type: !15)
!51 = !DILocation(line: 11, column: 12, scope: !46)
!52 = !DILocalVariable(name: "a", scope: !46, file: !1, line: 12, type: !14)
!53 = !DILocation(line: 12, column: 13, scope: !46)
!54 = !DILocalVariable(name: "b", scope: !46, file: !1, line: 13, type: !14)
!55 = !DILocation(line: 13, column: 13, scope: !46)
!56 = !DILocation(line: 14, column: 7, scope: !46)
!57 = !DILocation(line: 15, column: 13, scope: !46)
!58 = !DILocation(line: 15, column: 16, scope: !46)
!59 = !DILocation(line: 15, column: 7, scope: !46)
!60 = !DILocation(line: 16, column: 19, scope: !46)
!61 = !DILocation(line: 16, column: 14, scope: !46)
!62 = !DILocation(line: 16, column: 7, scope: !46)
