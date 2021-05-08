A = readSVG('bunny.svg');
A = A{1};
A(:,2) = -A(:,2);
[cen,vol] = centroid(A(1:end-1,:),[1:size(A,1)-1;2:size(A,1)-1 1]');
A = bsxfun(@minus,A,cen);
A = uniformly_sample(A,size(A,1));

B = A;

% X = [th cen p np a1 a2];

%X0 = [0   [0 0]   [0 1] 0 0 ];
%LB = [-pi min(A) -1 -1 -pi/2 -pi/2];
%UB = [ pi max(A)  1  1  pi/2 pi/2];
%f = @(X) -find_max_scale( ...
%  A,B*[cos(X(1)) sin(X(1));-sin(X(1)) cos(X(1))],X(2:3),X(2:3),X(4:5),X(6:7));

X0 = [0   [0 0]   [0 1]];
LB = [-pi min(A) -1 -1 ];
UB = [ pi max(A)  1  1 ];
f = @(X) -find_max_scale( ...
  A,B*[cos(X(1)) sin(X(1));-sin(X(1)) cos(X(1))],X(2:3),X(2:3),X(4:5),[0 0]);

options = optimoptions('particleswarm','InitialSwarmMatrix',X0);
tic;
%[X,s] = particleswarm(f,size(X0,2),LB,UB,options);
% Genetic algorithm is actually working better
[X,s] = ga(f,size(X0,2),[],[],[],[],LB,UB);
%% Simulated annealing 
%[X,s] = simulannealbnd(f,X0,LB,UB);

if numel(X) == 7 && f([X(1:5) 0 0]) == f(X)
  fprintf('Angle of attack not used')
  X(6:7) = 0;
end

s = -f(X);
th = X(1);
cen = X(2:3)
p = X(2:3);
np = X(4:5);
if numel(X)>=6 
  a1 = X(6);
  a2 = X(7);
else
  a1 = 0;
  a2 = 0;
end

toc


clf;
hold on;
colormap(parula(256));
plot(A(:,1),A(:,2),'LineWidth',4);
BB = s*B*[cos(th) sin(th);-sin(th) cos(th)]+cen;
plot(BB(:,1),BB(:,2),'LineWidth',4);
quiver(p(:,1),p(:,2), np(:,2),-np(:,1),40,'--k','LineWidth',1);
quiver(p(:,1),p(:,2),-np(:,2), np(:,1),40,'--k','LineWidth',1);
t1 = -np*[cos(a1) sin(a1);-sin(a1) cos(a1)];
t2 =  np*[cos(a2) sin(a2);-sin(a2) cos(a2)];
quiver(p(:,1),p(:,2),t1(:,1),t1(:,2),10,'r','LineWidth',2);
quiver(p(:,1),p(:,2),t2(:,1),t2(:,2),10,'b','LineWidth',2);
hold off;
axis equal;
drawnow;
title(sprintf('max(S): %g',s),'FontSize',30);
