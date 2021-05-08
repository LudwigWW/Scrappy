%A = readSVG('bunny.svg');
%A = A{1};
%A(:,2) = -A(:,2);
%[cen,vol] = centroid(A(1:end-1,:),[1:size(A,1)-1;2:size(A,1)-1 1]');
%A = bsxfun(@minus,A,cen);
%A = uniformly_sample(A,size(A,1));
%
%
%
%
%tic;
%w = 256
%num_thetas = 48
%% num_t_angles should be odd so that 0 shows up
%num_t_angles = 1;
%early_exit = false;
%
%[X,Y] = meshgrid( ...
%  linspace(min(A(:,1)),max(A(:,1)),w), ...
%  linspace(min(A(:,2)),max(A(:,2)),w));
%
%B = A;
%thetas = linspace(0,2*pi,num_thetas+1);
%thetas = thetas(1:end-1);
%
%t_angles = linspace(-pi/2,pi/2,num_t_angles+2);
%t_angles = t_angles(2:end-1);
%% Sort angles so that ties are broken in favor of less extreme angles
%[~,I] = sort(abs(t_angles));
%t_angles = t_angles(I);
%
%S = -inf(size(X));
%TH = nan(size(X));
%TA = nan([size(X) 2]);
%max_s = 0;
%% TODO: This is find for visualization, but for the actual search
%% find_max_scale should take the current max scale as input so that it can
%% immediately test if it can exit early.
%XI = 1:size(X,2);
%YI = 1:size(X,1);
%%XI = XI(randperm(end));
%%YI = YI(randperm(end));
%for xi = 1:size(X,2)
%  x = XI(xi);
%  for yi = 1:size(X,1)
%    y = YI(yi);
%    progressbar((xi-1)*size(X,1)+(yi-1),numel(X),40);
%    cen = [X(x,y) Y(x,y)];
%    % clipping plane
%    % Defaulting to centroid of B makes more sense than centroid of A
%    p = cen;
%    np = [0 1];
%
%    for th = thetas
%      for a1 = t_angles
%        for a2 = t_angles
%          s = find_max_scale( ...
%            A,B*[cos(th) sin(th);-sin(th) cos(th)],cen,p,np,[a1 a2], ...
%            'MaxScale',early_exit*max_s,'MinScale',0);
%          if s > S(x,y)
%            S(x,y) = s;
%            TH(x,y) = th;
%            TA(x,y,1) = a1;
%            TA(x,y,2) = a2;
%          end
%          if s > max_s
%            max_s = s;
%          end
%        end
%      end
%    end
%  end
%
%  %clf;
%  %hold on;
%  %surf(X,Y,0*S,'CData',sqrt(abs(S)),'EdgeColor','k',fphong);
%  %plot(A(:,1),A(:,2));
%  %plot(S(x,y)*B(:,1)+cen(1),S(x,y)*B(:,2)+cen(2));
%  %hold off;
%  %axis equal;
%  %drawnow;
%end
%toc
%save(sprintf('landscape-%d-%d.mat',w,num_thetas));

clear all
load('landscape-256-256.mat');
[x,y] = find(S==max(S(:)),1,'first');
cen = [X(x,y) Y(x,y)];
clf;
hold on;
surf(X,Y,0*S,'CData',S,'EdgeColor','none',fphong);
%colormap([1 1 1;(cbrewer('Oranges',256))]);
colormap([1 1 1;flipud(cbrewer('RdYlBu',256))]);
%plot(A(:,1),A(:,2),'LineWidth',4);
%BB = S(x,y)*B*[cos(TH(x,y)) sin(TH(x,y));-sin(TH(x,y)) cos(TH(x,y))]+cen;
%plot(BB(:,1),BB(:,2),'LineWidth',4);
%p = cen;
%quiver(p(:,1),p(:,2), np(:,2),-np(:,1),40,'--k','LineWidth',1);
%quiver(p(:,1),p(:,2),-np(:,2), np(:,1),40,'--k','LineWidth',1);
%t1 = -np*[cos(TA(x,y,1)) sin(TA(x,y,1));-sin(TA(x,y,1)) cos(TA(x,y,1))];
%t2 =  np*[cos(TA(x,y,2)) sin(TA(x,y,2));-sin(TA(x,y,2)) cos(TA(x,y,2))];
%quiver(p(:,1),p(:,2),t1(:,1),t1(:,2),10,'r','LineWidth',2);
%quiver(p(:,1),p(:,2),t2(:,1),t2(:,2),10,'b','LineWidth',2);
hold off;
axis equal;
drawnow;
title(sprintf('max(S): %g',S(x,y)),'FontSize',30);
view(2);
set(gcf,'Color','w');set(gca,'Visible','off');
