@echo off
set count=1000
set /p count=Set entity count (default - 1000):
set radius=50
set /p radius=Set arena radius (default - 50):
set loss=50
set /p loss=Set packet loss percentage (default - 50):
set delay=10
set /p delay=Set packet delay percentage (default - 10):
set range=1
set /p range=Set packet delay range (default - 1):

ServerApplication_d.exe -count %count% -radius %radius% -loss %loss% -delay %delay% -range %range%