%A = readSVG('bunny.svg');
%A = A{1};
%A(:,2) = -A(:,2);
%[cen,vol] = centroid(A(1:end-1,:),[1:size(A,1)-1;2:size(A,1)-1 1]');
%A = A-cen;

B = 0.8*A;
tic;
[I,~,~,X,Y] = lineSegmentIntersect( [A(1:end-2,:) A(2:end-1,:)], [B(1:end-2,:) B(2:end-1,:)]);
toc

X = X(I);
Y = Y(I);

numel(X)

clf;
hold on;
plot(A(:,1),A(:,2),'LineWidth',2);
plot(B(:,1),B(:,2),'LineWidth',2);
scatter(X(:),Y(:),'or');
hold off;
axis equal;
