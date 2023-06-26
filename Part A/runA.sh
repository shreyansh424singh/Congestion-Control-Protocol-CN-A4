./waf --run "scratch/First --tcp=TcpNewReno" 
./waf --run "scratch/First --tcp=TcpNewRenoPlus" 

gnuplot congestion1.plt
gnuplot congestion2.plt
gnuplot congestion3.plt
gnuplot congestion4.plt
gnuplot congestion5.plt
gnuplot congestion6.plt