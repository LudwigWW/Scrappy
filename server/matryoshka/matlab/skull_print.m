%% Remember the tolerance σ is add padding on "all sides". 
%sigma = 0.7;
%epsilon = 0.5;
%[VA,FA] = load_mesh('../examples/90/Skull_s.stl');
%[VA,~,I] = remove_duplicate_vertices(VA,1e-7);
%FA = I(FA);
%VA = (VA - min(VA))/max(max(VA)-min(VA));
%% Scale to be 10cm = 100mm
%VA = VA*100;
%VB = VA;
%FB = FA;
%
%iter = 8;
%iter_name = sprintf('skull-%d.mat',iter);
%
%% shrink by last scale
%[~,~,~,s] = skull_transformations(iter-1);
%VA = VA*s;
%
%% Create mesh of inner shell of A
%tic;
%[VI,FI] = signed_distance_isosurface( ...
%  VA,FA,'Level',-(sigma+epsilon),'ContouringMethod','cgal', ...
%  'RadiusBound',0.010,'DistanceBound',0.010, ...
%  'SignedDistanceType','pseudonormal');
%writePLY('A.ply',VI,FI);
%writePLY('B.ply',VB,FB);
%fprintf('Wrote {A,B}.ply; Please fill in model for number "case %d"\n',iter);
%pause

[model,p,np,s] = skull_transformations(iter);
% Transformed B
VBT = VB*model(1:3,1:3)' + model(1:3,4)';
tic;
[VO,FO] = signed_distance_isosurface( ...
  VBT,FB,'Level',sigma,'ContouringMethod','cgal', ...
  'RadiusBound',0.010,'DistanceBound',0.010, ...
  'SignedDistanceType','pseudonormal');
toc;
tic;
[VL,FL,JL,VR,FR,JR] = matryoshka(VA,FA,VO,FO,p,np);
toc
save(iter_name,'s','VI','FI','VO','FO','VL','FL','JL','VR','FR','JR');
writeOBJ(sprintf('skull-L%d.obj',iter),VL,FL);
writeOBJ(sprintf('skull-R%d.obj',iter),VR,FR);
writeOBJ(sprintf('skull-%d.obj',iter+1),VBT,FB);
