%[VA,FA] = load_mesh('../examples/50/MalteseFalconSolid.obj');
%[VA,~,I] = remove_duplicate_vertices(VA,1e-7);
%FA = I(FA);
%% Scale to be 10cm = 100mm
%VA = (VA - min(VA))/max(max(VA)-min(VA));
%VA = VA*100;
%[VB,FB] = load_mesh('../examples/key-bounding.obj');
%%
%%% Remember the tolerance σ is add padding on "all sides". 
%%sigma = 0.7;
%%epsilon = 0.5;
%%% Create mesh of inner shell of A
%%%tic;
%%%[VI,FI] = signed_distance_isosurface( ...
%%%  VA,FA,'Level',-(sigma+epsilon),'ContouringMethod','marching_cubes', ...
%%%  'GridSize',80, ...
%%%  'SignedDistanceType','winding_number');
%%%writePLY('mA.ply',VI,FI);
%%%writePLY('mB.ply',VB,FB);
%%%fprintf('Wrote {mA,mB}.ply; Please fill in model \n');
%%%model = [
%%%  0.8660918  0.2300762 0.04328769    19.8093
%%%  -0.2238136  0.8623753 -0.1055473   23.83192
%%%  -0.0686757 0.09109172  0.8898932   52.31253
%%%  0          0          0          1
%%%];
%%%p = [
%%%  19.82709 24.37019 50.77182
%%%];
%%%np = [
%%%  -0.04228157  -0.9981312 -0.04411759
%%%];
%%%s = 0.897176;
%model = [
%  -0.7444587   0.4297399 -0.03679729    19.75764
%  -0.4272206  -0.7447945 -0.05488972    26.65169
%  -0.05927018 -0.02922272   0.8578359     53.1362
%  0           0           0           1
%];
%p = [
%  19.99726 26.44517 51.60332
%];
%np = [
%  0 -0  1
%];
%s = 0.860377;
%
%VBT = VB*model(1:3,1:3)' + model(1:3,4)';
%%tsurf(FB,VBT);
%%error
%%tic;
%%[VL,FL,JL,VR,FR,JR] = matryoshka(VA,FA,VBT,FB,p,np);
%%toc
%%tsurf(FL,VL);
%%axis equal;
%%writeOBJ('maltese-L.obj',VL,FL);
%%writeOBJ('maltese-R.obj',VR,FR);
%%writeOBJ('maltese-B.obj',VB*s,FB);
%%[VB,FB] = load_mesh('../examples/key.obj');
%%writeOBJ('maltese-B-orig.obj',VB*s,FB);
%
%[VL,FL]= load_mesh('maltese-L.obj');
%[VR,FR]= load_mesh('maltese-R.obj');
[VB,FB] = load_mesh('../examples/key.obj');
VBT = VB*model(1:3,1:3)' + model(1:3,4)';

msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.3,'AmbientStrength',0.6,'SpecularExponent',100};
  ts = {};
  ks = {};
clf;
hold on;
for pass = 1:2
  switch pass
  case 2
    f = 100;
  case 1
    f = 10;
  end
  slide = (pass-1)*[50 00 0];
  ts{end+1} = tsurf(FR,slide+     VR+f*np,'FaceVertexCData',repmat(0.3*[1 1 1],size(FR,1),1),'EdgeColor','none',msoft{:});
  ts{end+1} = tsurf(FL,slide+          VL,'FaceVertexCData',repmat(0.3*[1 1 1],size(FL,1),1),'EdgeColor','none',msoft{:});
  ks{end+1} = tsurf(FB,slide+VBT+0.5*f*np,'FaceColor',[0.8 0.8 1],'EdgeColor','none','FaceAlpha',0.5,fsoft);
end
hold off;
l = light('Position',[-100 -30 500],'Style','infinite');
add_shadow(ts,l,'Color',0.95*[1 1 1]);
%add_shadow(ks,l,'Color',[0.9 0.9 1],'Ground',[0 0 -1 min(VL(:,3))+1]);
view(-23,11);
camlight;
axis equal;
apply_ambient_occlusion([ts{:}]);
for ti = 1:numel(ts)
  ts{ti}.SpecularStrength = 0.8;
end
camproj('persp');
set(gca,'pos',[0 0 1 1])
set(gca,'Visible','off');set(gcf,'Color','w');
