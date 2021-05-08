% Remember the tolerance σ is add padding on "all sides". 
sigma = 0.7;
epsilon = 0.5;
[VA,FA] = load_mesh('../examples/70/gargoyle-ball-decimated.obj');
[VA,~,I] = remove_duplicate_vertices(VA,1e-7);
FA = I(FA);
VA = (VA - min(VA))/max(max(VA)-min(VA));
% Scale to be 10cm = 100mm
VA = VA*100;
VB = VA;
FB = FA;

iter = 3;

% shrink by last scale
[~,~,~,s] = gargoyle_ball_transformations(iter-1);
VA = VA*s;

% Create mesh of inner shell of A
tic;
[VI,FI] = signed_distance_isosurface( ...
  VA,FA,'Level',-(sigma+epsilon),'ContouringMethod','marching_cubes', ...
  'GridSize',80, ...
  'SignedDistanceType','pseudonormal');
writePLY('A.ply',VI,FI);
writePLY('B.ply',VB,FB);
fprintf('Wrote {A,B}.ply; Please fill in model for number "case %d"\n',iter);
pause

%[model,p,np,s] = gargoyle_ball_transformations(iter);
%
%% Transformed B
%VBT = VB*model(1:3,1:3)' + model(1:3,4)';
%
%clf;
%hold on;
%tsurf(FA,VA,'FaceColor',blue,'FaceAlpha',0.2,'EdgeColor','none');
%tsurf(FB,VBT,'FaceColor',orange,'FaceAlpha',0.2,'EdgeColor','none');
%hold off;
%camlight;
%axis equal;
%drawnow;
%
%tic;
%[VO,FO] = signed_distance_isosurface( ...
%  VBT,FB,'Level',-(sigma+epsilon),'ContouringMethod','marching_cubes', ...
%  'GridSize',60, ...
%  'SignedDistanceType','pseudonormal');
%toc;
%assert(isempty(outline(FO)),'FO should not have boundaries');
%
%tic;
%[VL,FL,JL,VR,FR,JR] = matryoshka(VA,FA,VO,FO,p,np);
%toc
%iter_name = sprintf('gargoyle-ball-%d.mat',iter);
%save(iter_name,'s','VI','FI','VO','FO','VL','FL','JL','VR','FR','JR');
%writeOBJ(sprintf('gargoyle-ball-L%d.obj',iter),VL,FL);
%writeOBJ(sprintf('gargoyle-ball-R%d.obj',iter),VR,FR);
%writeOBJ(sprintf('gargoyle-ball-%d.obj',iter+1),VBT,FB);
%
