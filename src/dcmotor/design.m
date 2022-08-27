pkg load control
pkg load symbolic

# Start with a continuous time state space model
syms s J b K R L z Ts
A = [-b/J   K/J;
    -K/L   -R/L];
B = [0;
    1/L];
C = [1   0];
D = 0;

# Substitute variables for values
A = double(subs(A, [J b K R L], [0.01, 0.001, 0.01, 1, 0.5]))
B = double(subs(B, [J b K R L], [0.01, 0.001, 0.01, 1, 0.5]))

# Convert to continuous time state space model object
sys = ss(A, B, C, D);

% Determine observability and controllability
e = eig(A) % all must be negative!
assert(e(1) < 0 && e(2) < 0)
M_o = obsv(sys)
M_c = ctrb(sys)
assert(length(A) - rank(M_o) == 0)
assert(length(A) - rank(M_c) == 0)

sys = c2d(sys, 0.1)
% Design controller
Q = [9 0; 0 1]
R = [1]
sys
[sys_k, K] = kalman(sys, 2.3, 1)
[L, S, P] = lqr(sys, Q, R)
dc_g = dcgain(sys)

% Reference gain
N = -(C*(A-B*L)^-1*B)^-1

A_cl = A-B*L;

tf(ss(A_cl, B, C, D))

% Observer
op = 5 * P;
L = place(A', C', op)'
N2 = -([C 0 0]*([A -B*K; L*C A-B*K-L*C]^-1)*[B; B])^-1

assert(N == N2)
