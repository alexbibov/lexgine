@ECHO OFF

dot ../x64/Debug/task_graph.gv -Tpdf > task_graph.pdf

for /r %%i in (../x64/Debug/deferred_pso_compilation_task_graph__*.gv) do dot ../x64/Debug/%%~ni%%~xi -Tpdf > %%~ni.pdf
for /r %%i in (../x64/Debug/streamed_cache_index_tree__*.gv) do dot ../x64/Debug/%%~ni%%~xi -Tpdf > %%~ni.pdf