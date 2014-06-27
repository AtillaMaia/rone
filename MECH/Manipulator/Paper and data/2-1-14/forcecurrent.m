function forcecurrent
f = [0
0
2.549729
2.941995
2.2555295
1.96133
2.745862
2.157463
2.353596
0.6864655
0.8825985
1.176798
1.372931
0.588399
0.8825985
1.176798
1.4709975
1.765197
2.0593965
1.0787315
0.980665
0.4903325
1.2748645
1.569064
1.96133
2.353596
2.6477955
1.2748645
1.6671305
1.8632635
2.4516625
0.784532
0.980665
1.2748645
1.765197
3.138128
2.0593965
1.2748645
1.96133
2.353596
1.8632635
2.2555295
2.2555295
2.157463
0
0.2941995
0.588399
0.8825985
0.980665
1.176798
1.372931
1.6671305
0.0980665
0.196133
0.392266
0.4903325
0.6864655
0.784532
0.980665
1.372931
1.569064
1.6671305
2.2555295
1.8632635
1.2748645
0.2941995
0.588399];

c = [0.005
0.005
0.244
0.381
0.258
0.19
0.377
0.255
0.287
0.048
0.151
0.263
0.385
0.062
0.135
0.161
0.181
0.187
0.226
0.091
0.138
0.077
0.172
0.212
0.2
0.306
0.385
0.188
0.218
0.264
0.366
0.148
0.168
0.179
0.203
0.388
0.325
0.176
0.272
0.362
0.311
0.355
0.287
0.301
0.005
0.042
0.125
0.134
0.174
0.199
0.224
0.236
0.048
0.085
0.107
0.104
0.135
0.158
0.165
0.196
0.238
0.257
0.378
0.365
0.259
0.096
0.162];

figure(1)
plot(f,c,'b.','MarkerSize',20)
title('Current Draw vs. Force Applied', 'fontsize', 18)
xlabel('Force (Newtons)', 'fontsize', 16)
ylabel('Current (Amps)', 'fontsize', 16)

return