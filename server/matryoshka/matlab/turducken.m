%%
%VV = {};
%FF = {};
%M = {};
%p = {};
%np = {};
%[VV{end+1},FF{end+1}] = load_mesh('../examples/turducken/turkey.ply');
%[VV{end+1},FF{end+1}] = load_mesh('../examples/turducken/duck.ply');
%[VV{end+1},FF{end+1}] = load_mesh('../examples/turducken/chicken-decimated.ply');
%M{end+1} = [
%    -0.4528415    0.2082508  0.003061473    0.5492443
%    -0.2081271   -0.4527492    0.0120187    0.6603135
%   0.007802298  0.009640845    0.4982865 -0.002676614
%             0            0            0            1
%];
%p{end+1} = [
%  0.4830314 0.3954248  0.179054
%];
%np{end+1} = [
%  -0  0  1
%];
%M{end+1} = [
%    0.4354777   0.6286215   0.0635965 -0.02994996
%    0.1330572 -0.01573507  -0.7555783   0.8555393
%   -0.6176612   0.4398158  -0.1179294   0.3324093
%            0           0           0           1
%];
%p{end+1} = [
%  0.3372191 0.5162184 0.2547591
%];
%np{end+1} = [
%  -0.3335153 -0.9036291 -0.2687415
%];
%
%VL = {};
%VR = {};
%FL = {};
%FR = {};
%JL = {};
%JR = {};
%for k = 1:numel(VV)-1
%  VT = [VV{k+1} ones(size(VV{k+1},1),1)]*M{k}(1:3,:)';
%  [VO,FO] = signed_distance_isosurface( ...
%    VT,FF{k+1},'Level',0.0001,'ContouringMethod','marching_cubes', ...
%    'SignedDistanceType','winding_number');
%  [VL{k},FL{k},JL{k},VR{k},FR{k},JR{k}] = matryoshka( ...
%    VV{k},FF{k},VO,FO,p{k},np{k});
%end
%
%save('turducken.mat');

%off = [0.9 0 0];
%t = {};
%params = {'EdgeColor','none',fsoft};
%MM = {eye(4),M{:}};
%f = 0.6;
%C = [0.5 0.3 0.3;orange;1 1 0;0.6*[1 0.95 0.9]];
%R = axisangle2matrix([0 0 1],-pi*0.4);
%clf;
%hold on;
%for K = 1:3
%  MT = eye(4);
%  MTL = eye(4);
%  MTR = eye(4);
%  for k = 1:K
%    MT = MT * MM{k};
%    if k==K
%      Ck = k*ones(size(FF{k},1),1);
%      t{end+1} = tsurf(FF{k},[VV{k} ones(size(VV{k},1),1)]*MT(1:3,:)' *R+K*off,params{:} ,'CData',Ck);
%    else
%      if k == K-1
%        fk = f*0.9;
%      else 
%        fk = f;
%      end
%      MTL = MT *[eye(3)  fk*np{k}';0 0 0 1];
%      MTR = MT *[eye(3) -fk*np{k}';0 0 0 1];
%      CL = (JL{k}<size(FF{k},1)).*k + (JL{k}>=size(FF{k},1)).*size(C,1);
%      CR = (JR{k}<size(FF{k},1)).*k + (JR{k}>=size(FF{k},1)).*size(C,1);
%      t{end+1} = tsurf(FR{k},[VR{k} ones(size(VR{k},1),1)]*MTL(1:3,:)'*R+K*off,params{:},'CData',CR);
%      t{end+1} = tsurf(FL{k},[VL{k} ones(size(VL{k},1),1)]*MTR(1:3,:)'*R+K*off,params{:},'CData',CL);
%    end
%  end
%end
%hold off;
%colormap(C);
%view(0,36);
%axis equal;
%camlight;
%l = light('Position',[4 -3 10],'Style','Infinite');
%caxis manual;
%add_shadow(t,l,'Color',0.95*[1 1 1]);
%cellfun(@(ti) set(ti,'SpecularExponent',20,'SpecularStrength',0.1),t);
%%apply_ambient_occlusion([],'AddLights',false,'SoftLighting',false);
%camproj('persp');
%set(gca,'Visible','off');
%set(gca,'pos',[0 0 1 1]);
%set(gcf,'Color','w');

load('turducken');
%AL = [];AR = [];
matryoshka_video( ...
  {VL{:},VV{end}},{FL{:},FF{end}},{JL{:},zeros(size(FF{end},1),1)}, ...
  {VR{:},[]},{FR{:},[]},{JR{:},[]}, ...
  M,np, ...
  'ColorMap',[0.5 0.3 0.3;orange;1 1 0], ...
  'Callback',@() figgif('turducken.gif'), ...
  'SliceColor',[0.6*[1 0.95 0.9]], ...
  'NumFaces',cellfun(@(f) size(f,1),FF), ...
  'LightParams',{'Position',[-0.3 0 1],'Style','infinite'}, ...
  'View',[100 38],'ViewChange',[183 -22], ...
  'AL',AL,'AR',AR);

