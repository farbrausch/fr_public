@echo off
release\dce ..\werkkzeug3 ..\werkkzeug3_lekktor test %1
copy ..\werkkzeug3_lekktor\%1.cpp before.cpp
release\dce ..\werkkzeug3 ..\werkkzeug3_lekktor post %1
copy ..\werkkzeug3_lekktor\%1.cpp after.cpp
windiff before.cpp after.cpp
del before.cpp
del after.cpp
