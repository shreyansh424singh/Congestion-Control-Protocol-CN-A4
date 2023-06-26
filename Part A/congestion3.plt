set terminal png
set output "TcpNewReno_congestion_N2.png"
set title "TcpNewReno Congestion Window N2"
set xlabel "Time (in Seconds)"
set ylabel "Congestion Window (cwnd)"
plot "TcpNewReno_N2_Source.cwnd" using 1:3 with linespoint title "Congestion Window"
