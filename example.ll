; ModuleID = 'example.c'
source_filename = "example.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.Node = type { i32, %struct.Node.0* }
%struct.Node.0 = type opaque

@.str = private unnamed_addr constant [6 x i8] c"%c %c\00", align 1
@.str.1 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @swap(i8** noundef %p, i8** noundef %q) #0 !dbg !20 {
entry:
  %p.addr = alloca i8**, align 8
  %q.addr = alloca i8**, align 8
  %t = alloca i8*, align 8
  store i8** %p, i8*** %p.addr, align 8
  call void @llvm.dbg.declare(metadata i8*** %p.addr, metadata !27, metadata !DIExpression()), !dbg !28
  store i8** %q, i8*** %q.addr, align 8
  call void @llvm.dbg.declare(metadata i8*** %q.addr, metadata !29, metadata !DIExpression()), !dbg !30
  call void @llvm.dbg.declare(metadata i8** %t, metadata !31, metadata !DIExpression()), !dbg !32
  %0 = load i8**, i8*** %p.addr, align 8, !dbg !33
  %1 = load i8*, i8** %0, align 8, !dbg !34
  store i8* %1, i8** %t, align 8, !dbg !32
  %2 = load i8**, i8*** %q.addr, align 8, !dbg !35
  %3 = load i8*, i8** %2, align 8, !dbg !36
  %4 = load i8**, i8*** %p.addr, align 8, !dbg !37
  store i8* %3, i8** %4, align 8, !dbg !38
  %5 = load i8*, i8** %t, align 8, !dbg !39
  %6 = load i8**, i8*** %q.addr, align 8, !dbg !40
  store i8* %5, i8** %6, align 8, !dbg !41
  ret void, !dbg !42
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @print(i8* noundef %a, i8* noundef %b) #0 !dbg !43 {
entry:
  %a.addr = alloca i8*, align 8
  %b.addr = alloca i8*, align 8
  store i8* %a, i8** %a.addr, align 8
  call void @llvm.dbg.declare(metadata i8** %a.addr, metadata !46, metadata !DIExpression()), !dbg !47
  store i8* %b, i8** %b.addr, align 8
  call void @llvm.dbg.declare(metadata i8** %b.addr, metadata !48, metadata !DIExpression()), !dbg !49
  %0 = load i8*, i8** %a.addr, align 8, !dbg !50
  %1 = load i8, i8* %0, align 1, !dbg !51
  %conv = sext i8 %1 to i32, !dbg !51
  %2 = load i8*, i8** %b.addr, align 8, !dbg !52
  %3 = load i8, i8* %2, align 1, !dbg !53
  %conv1 = sext i8 %3 to i32, !dbg !53
  %call = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([6 x i8], [6 x i8]* @.str, i64 0, i64 0), i32 noundef %conv, i32 noundef %conv1), !dbg !54
  ret void, !dbg !55
}

declare i32 @printf(i8* noundef, ...) #2

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i8* @identity(i8* noundef %x) #0 !dbg !56 {
entry:
  %x.addr = alloca i8*, align 8
  store i8* %x, i8** %x.addr, align 8
  call void @llvm.dbg.declare(metadata i8** %x.addr, metadata !60, metadata !DIExpression()), !dbg !61
  %0 = load i8*, i8** %x.addr, align 8, !dbg !62
  ret i8* %0, !dbg !63
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @returnConst5() #0 !dbg !64 {
entry:
  ret i32 5, !dbg !67
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local %struct.Node* @createNode(i32 noundef %val) #0 !dbg !68 {
entry:
  %val.addr = alloca i32, align 4
  %newNode = alloca %struct.Node*, align 8
  store i32 %val, i32* %val.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %val.addr, metadata !70, metadata !DIExpression()), !dbg !71
  call void @llvm.dbg.declare(metadata %struct.Node** %newNode, metadata !72, metadata !DIExpression()), !dbg !73
  %call = call noalias i8* @malloc(i64 noundef 16) #4, !dbg !74
  %0 = bitcast i8* %call to %struct.Node*, !dbg !75
  store %struct.Node* %0, %struct.Node** %newNode, align 8, !dbg !73
  %1 = load i32, i32* %val.addr, align 4, !dbg !76
  %2 = load %struct.Node*, %struct.Node** %newNode, align 8, !dbg !77
  %data = getelementptr inbounds %struct.Node, %struct.Node* %2, i32 0, i32 0, !dbg !78
  store i32 %1, i32* %data, align 8, !dbg !79
  %3 = load %struct.Node*, %struct.Node** %newNode, align 8, !dbg !80
  ret %struct.Node* %3, !dbg !81
}

; Function Attrs: nounwind
declare noalias i8* @malloc(i64 noundef) #3

; Function Attrs: noinline nounwind optnone uwtable
define dso_local %struct.Node* @extremeIdentityFunction(%struct.Node* noundef %b) #0 !dbg !82 {
entry:
  %b.addr = alloca %struct.Node*, align 8
  %a = alloca %struct.Node, align 8
  %x = alloca %struct.Node*, align 8
  %y = alloca %struct.Node*, align 8
  store %struct.Node* %b, %struct.Node** %b.addr, align 8
  call void @llvm.dbg.declare(metadata %struct.Node** %b.addr, metadata !85, metadata !DIExpression()), !dbg !86
  call void @llvm.dbg.declare(metadata %struct.Node* %a, metadata !87, metadata !DIExpression()), !dbg !88
  %0 = bitcast %struct.Node** %b.addr to %struct.Node.0*, !dbg !89
  %next = getelementptr inbounds %struct.Node, %struct.Node* %a, i32 0, i32 1, !dbg !90
  store %struct.Node.0* %0, %struct.Node.0** %next, align 8, !dbg !91
  call void @llvm.dbg.declare(metadata %struct.Node** %x, metadata !92, metadata !DIExpression()), !dbg !93
  %1 = bitcast %struct.Node* %a to i8*, !dbg !94
  %call = call i8* @identity(i8* noundef %1), !dbg !95
  %2 = bitcast i8* %call to %struct.Node*, !dbg !95
  store %struct.Node* %2, %struct.Node** %x, align 8, !dbg !93
  call void @llvm.dbg.declare(metadata %struct.Node** %y, metadata !96, metadata !DIExpression()), !dbg !97
  %3 = load %struct.Node*, %struct.Node** %x, align 8, !dbg !98
  %next1 = getelementptr inbounds %struct.Node, %struct.Node* %3, i32 0, i32 1, !dbg !99
  %4 = load %struct.Node.0*, %struct.Node.0** %next1, align 8, !dbg !99
  %5 = bitcast %struct.Node.0* %4 to %struct.Node*, !dbg !98
  store %struct.Node* %5, %struct.Node** %y, align 8, !dbg !97
  %6 = load %struct.Node*, %struct.Node** %y, align 8, !dbg !100
  ret %struct.Node* %6, !dbg !101
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @printNodeVale(%struct.Node* noundef %n) #0 !dbg !102 {
entry:
  %n.addr = alloca %struct.Node*, align 8
  store %struct.Node* %n, %struct.Node** %n.addr, align 8
  call void @llvm.dbg.declare(metadata %struct.Node** %n.addr, metadata !105, metadata !DIExpression()), !dbg !106
  %0 = load %struct.Node*, %struct.Node** %n.addr, align 8, !dbg !107
  %data = getelementptr inbounds %struct.Node, %struct.Node* %0, i32 0, i32 0, !dbg !108
  %1 = load i32, i32* %data, align 8, !dbg !108
  %call = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.1, i64 0, i64 0), i32 noundef %1), !dbg !109
  ret void, !dbg !110
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !111 {
entry:
  %retval = alloca i32, align 4
  %a1 = alloca i8, align 1
  %b1 = alloca i8, align 1
  %a = alloca i8*, align 8
  %b = alloca i8*, align 8
  %constfive = alloca i32, align 4
  %n = alloca %struct.Node*, align 8
  %nn = alloca %struct.Node*, align 8
  store i32 0, i32* %retval, align 4
  call void @llvm.dbg.declare(metadata i8* %a1, metadata !112, metadata !DIExpression()), !dbg !113
  store i8 107, i8* %a1, align 1, !dbg !113
  call void @llvm.dbg.declare(metadata i8* %b1, metadata !114, metadata !DIExpression()), !dbg !115
  store i8 108, i8* %b1, align 1, !dbg !115
  call void @llvm.dbg.declare(metadata i8** %a, metadata !116, metadata !DIExpression()), !dbg !117
  store i8* %a1, i8** %a, align 8, !dbg !117
  call void @llvm.dbg.declare(metadata i8** %b, metadata !118, metadata !DIExpression()), !dbg !119
  store i8* %b1, i8** %b, align 8, !dbg !119
  call void @swap(i8** noundef %a, i8** noundef %b), !dbg !120
  %0 = load i8*, i8** %a, align 8, !dbg !121
  %1 = load i8*, i8** %b, align 8, !dbg !122
  call void @print(i8* noundef %0, i8* noundef %1), !dbg !123
  call void @llvm.dbg.declare(metadata i32* %constfive, metadata !124, metadata !DIExpression()), !dbg !125
  %call = call i32 @returnConst5(), !dbg !126
  store i32 %call, i32* %constfive, align 4, !dbg !125
  call void @llvm.dbg.declare(metadata %struct.Node** %n, metadata !127, metadata !DIExpression()), !dbg !128
  %2 = load i32, i32* %constfive, align 4, !dbg !129
  %call1 = call %struct.Node* @createNode(i32 noundef %2), !dbg !130
  store %struct.Node* %call1, %struct.Node** %n, align 8, !dbg !128
  call void @llvm.dbg.declare(metadata %struct.Node** %nn, metadata !131, metadata !DIExpression()), !dbg !132
  %3 = load %struct.Node*, %struct.Node** %n, align 8, !dbg !133
  %call2 = call %struct.Node* @extremeIdentityFunction(%struct.Node* noundef %3), !dbg !134
  store %struct.Node* %call2, %struct.Node** %nn, align 8, !dbg !132
  %4 = load %struct.Node*, %struct.Node** %nn, align 8, !dbg !135
  %call3 = call i32 (%struct.Node*, ...) bitcast (i32 (...)* @printNodeValue to i32 (%struct.Node*, ...)*)(%struct.Node* noundef %4), !dbg !136
  %5 = load i8*, i8** %a, align 8, !dbg !137
  %6 = ptrtoint i8* %5 to i32, !dbg !138
  ret i32 %6, !dbg !139
}

declare i32 @printNodeValue(...) #2

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!12, !13, !14, !15, !16, !17, !18}
!llvm.ident = !{!19}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "Ubuntu clang version 14.0.6", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "example.c", directory: "/home/julius/SVF-Java", checksumkind: CSK_MD5, checksum: "cfd8183adfd5d4e0bef17c61a82d7f35")
!2 = !{!3, !8}
!3 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!4 = !DIDerivedType(tag: DW_TAG_typedef, name: "Node", file: !1, line: 6, baseType: !5)
!5 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !1, line: 3, size: 128, elements: !6)
!6 = !{!7, !9}
!7 = !DIDerivedType(tag: DW_TAG_member, name: "data", scope: !5, file: !1, line: 4, baseType: !8, size: 32)
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!9 = !DIDerivedType(tag: DW_TAG_member, name: "next", scope: !5, file: !1, line: 5, baseType: !10, size: 64, offset: 64)
!10 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !11, size: 64)
!11 = !DICompositeType(tag: DW_TAG_structure_type, name: "Node", file: !1, line: 5, flags: DIFlagFwdDecl)
!12 = !{i32 7, !"Dwarf Version", i32 5}
!13 = !{i32 2, !"Debug Info Version", i32 3}
!14 = !{i32 1, !"wchar_size", i32 4}
!15 = !{i32 7, !"PIC Level", i32 2}
!16 = !{i32 7, !"PIE Level", i32 2}
!17 = !{i32 7, !"uwtable", i32 1}
!18 = !{i32 7, !"frame-pointer", i32 2}
!19 = !{!"Ubuntu clang version 14.0.6"}
!20 = distinct !DISubprogram(name: "swap", scope: !1, file: !1, line: 7, type: !21, scopeLine: 7, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !26)
!21 = !DISubroutineType(types: !22)
!22 = !{null, !23, !23}
!23 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !24, size: 64)
!24 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !25, size: 64)
!25 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!26 = !{}
!27 = !DILocalVariable(name: "p", arg: 1, scope: !20, file: !1, line: 7, type: !23)
!28 = !DILocation(line: 7, column: 18, scope: !20)
!29 = !DILocalVariable(name: "q", arg: 2, scope: !20, file: !1, line: 7, type: !23)
!30 = !DILocation(line: 7, column: 28, scope: !20)
!31 = !DILocalVariable(name: "t", scope: !20, file: !1, line: 8, type: !24)
!32 = !DILocation(line: 8, column: 9, scope: !20)
!33 = !DILocation(line: 8, column: 14, scope: !20)
!34 = !DILocation(line: 8, column: 13, scope: !20)
!35 = !DILocation(line: 9, column: 14, scope: !20)
!36 = !DILocation(line: 9, column: 13, scope: !20)
!37 = !DILocation(line: 9, column: 9, scope: !20)
!38 = !DILocation(line: 9, column: 11, scope: !20)
!39 = !DILocation(line: 10, column: 13, scope: !20)
!40 = !DILocation(line: 10, column: 9, scope: !20)
!41 = !DILocation(line: 10, column: 11, scope: !20)
!42 = !DILocation(line: 11, column: 1, scope: !20)
!43 = distinct !DISubprogram(name: "print", scope: !1, file: !1, line: 12, type: !44, scopeLine: 12, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !26)
!44 = !DISubroutineType(types: !45)
!45 = !{null, !24, !24}
!46 = !DILocalVariable(name: "a", arg: 1, scope: !43, file: !1, line: 12, type: !24)
!47 = !DILocation(line: 12, column: 18, scope: !43)
!48 = !DILocalVariable(name: "b", arg: 2, scope: !43, file: !1, line: 12, type: !24)
!49 = !DILocation(line: 12, column: 27, scope: !43)
!50 = !DILocation(line: 13, column: 24, scope: !43)
!51 = !DILocation(line: 13, column: 23, scope: !43)
!52 = !DILocation(line: 13, column: 28, scope: !43)
!53 = !DILocation(line: 13, column: 27, scope: !43)
!54 = !DILocation(line: 13, column: 7, scope: !43)
!55 = !DILocation(line: 14, column: 1, scope: !43)
!56 = distinct !DISubprogram(name: "identity", scope: !1, file: !1, line: 16, type: !57, scopeLine: 16, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !26)
!57 = !DISubroutineType(types: !58)
!58 = !{!59, !59}
!59 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!60 = !DILocalVariable(name: "x", arg: 1, scope: !56, file: !1, line: 16, type: !59)
!61 = !DILocation(line: 16, column: 22, scope: !56)
!62 = !DILocation(line: 17, column: 14, scope: !56)
!63 = !DILocation(line: 17, column: 7, scope: !56)
!64 = distinct !DISubprogram(name: "returnConst5", scope: !1, file: !1, line: 19, type: !65, scopeLine: 19, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !26)
!65 = !DISubroutineType(types: !66)
!66 = !{!8}
!67 = !DILocation(line: 20, column: 7, scope: !64)
!68 = distinct !DISubprogram(name: "createNode", scope: !1, file: !1, line: 22, type: !69, scopeLine: 22, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !26)
!69 = !DISubroutineType(types: !2)
!70 = !DILocalVariable(name: "val", arg: 1, scope: !68, file: !1, line: 22, type: !8)
!71 = !DILocation(line: 22, column: 22, scope: !68)
!72 = !DILocalVariable(name: "newNode", scope: !68, file: !1, line: 23, type: !3)
!73 = !DILocation(line: 23, column: 12, scope: !68)
!74 = !DILocation(line: 23, column: 29, scope: !68)
!75 = !DILocation(line: 23, column: 22, scope: !68)
!76 = !DILocation(line: 24, column: 22, scope: !68)
!77 = !DILocation(line: 24, column: 6, scope: !68)
!78 = !DILocation(line: 24, column: 15, scope: !68)
!79 = !DILocation(line: 24, column: 20, scope: !68)
!80 = !DILocation(line: 25, column: 13, scope: !68)
!81 = !DILocation(line: 25, column: 6, scope: !68)
!82 = distinct !DISubprogram(name: "extremeIdentityFunction", scope: !1, file: !1, line: 27, type: !83, scopeLine: 27, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !26)
!83 = !DISubroutineType(types: !84)
!84 = !{!3, !3}
!85 = !DILocalVariable(name: "b", arg: 1, scope: !82, file: !1, line: 27, type: !3)
!86 = !DILocation(line: 27, column: 37, scope: !82)
!87 = !DILocalVariable(name: "a", scope: !82, file: !1, line: 28, type: !4)
!88 = !DILocation(line: 28, column: 12, scope: !82)
!89 = !DILocation(line: 29, column: 16, scope: !82)
!90 = !DILocation(line: 29, column: 9, scope: !82)
!91 = !DILocation(line: 29, column: 14, scope: !82)
!92 = !DILocalVariable(name: "x", scope: !82, file: !1, line: 30, type: !3)
!93 = !DILocation(line: 30, column: 13, scope: !82)
!94 = !DILocation(line: 30, column: 26, scope: !82)
!95 = !DILocation(line: 30, column: 17, scope: !82)
!96 = !DILocalVariable(name: "y", scope: !82, file: !1, line: 31, type: !3)
!97 = !DILocation(line: 31, column: 13, scope: !82)
!98 = !DILocation(line: 31, column: 17, scope: !82)
!99 = !DILocation(line: 31, column: 20, scope: !82)
!100 = !DILocation(line: 32, column: 14, scope: !82)
!101 = !DILocation(line: 32, column: 7, scope: !82)
!102 = distinct !DISubprogram(name: "printNodeVale", scope: !1, file: !1, line: 35, type: !103, scopeLine: 35, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !26)
!103 = !DISubroutineType(types: !104)
!104 = !{null, !3}
!105 = !DILocalVariable(name: "n", arg: 1, scope: !102, file: !1, line: 35, type: !3)
!106 = !DILocation(line: 35, column: 26, scope: !102)
!107 = !DILocation(line: 36, column: 22, scope: !102)
!108 = !DILocation(line: 36, column: 25, scope: !102)
!109 = !DILocation(line: 36, column: 7, scope: !102)
!110 = !DILocation(line: 37, column: 1, scope: !102)
!111 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 38, type: !65, scopeLine: 38, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !26)
!112 = !DILocalVariable(name: "a1", scope: !111, file: !1, line: 39, type: !25)
!113 = !DILocation(line: 39, column: 12, scope: !111)
!114 = !DILocalVariable(name: "b1", scope: !111, file: !1, line: 40, type: !25)
!115 = !DILocation(line: 40, column: 12, scope: !111)
!116 = !DILocalVariable(name: "a", scope: !111, file: !1, line: 41, type: !24)
!117 = !DILocation(line: 41, column: 13, scope: !111)
!118 = !DILocalVariable(name: "b", scope: !111, file: !1, line: 42, type: !24)
!119 = !DILocation(line: 42, column: 13, scope: !111)
!120 = !DILocation(line: 43, column: 7, scope: !111)
!121 = !DILocation(line: 44, column: 13, scope: !111)
!122 = !DILocation(line: 44, column: 16, scope: !111)
!123 = !DILocation(line: 44, column: 7, scope: !111)
!124 = !DILocalVariable(name: "constfive", scope: !111, file: !1, line: 46, type: !8)
!125 = !DILocation(line: 46, column: 11, scope: !111)
!126 = !DILocation(line: 46, column: 23, scope: !111)
!127 = !DILocalVariable(name: "n", scope: !111, file: !1, line: 47, type: !3)
!128 = !DILocation(line: 47, column: 13, scope: !111)
!129 = !DILocation(line: 47, column: 28, scope: !111)
!130 = !DILocation(line: 47, column: 17, scope: !111)
!131 = !DILocalVariable(name: "nn", scope: !111, file: !1, line: 48, type: !3)
!132 = !DILocation(line: 48, column: 13, scope: !111)
!133 = !DILocation(line: 48, column: 42, scope: !111)
!134 = !DILocation(line: 48, column: 18, scope: !111)
!135 = !DILocation(line: 49, column: 22, scope: !111)
!136 = !DILocation(line: 49, column: 7, scope: !111)
!137 = !DILocation(line: 50, column: 19, scope: !111)
!138 = !DILocation(line: 50, column: 14, scope: !111)
!139 = !DILocation(line: 50, column: 7, scope: !111)
