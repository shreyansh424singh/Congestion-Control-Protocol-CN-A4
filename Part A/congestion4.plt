set terminal png
set output "TcpNewRenoPlus_congestion_N1_1.png"
set title "TcpNewRenoPlus Congestion Window N1 1"
set xlabel "Time (in Seconds)"
set ylabel "Congestion Window (cwnd)"
plot "TcpNewRenoPlus_N1_1_Source.cwnd" using 1:3 with linespoint title "Congestion Window"
