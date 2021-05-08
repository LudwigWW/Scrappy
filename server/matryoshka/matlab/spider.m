%[VA,FA] = load_mesh('../examples/30/spider_quad.obj');
%VB = VA;
%FB = FA;
%model = [
%  0.0887  -0.3584853 -0.03745896    1.399714
%  0.2663518  0.09118214  -0.2419197    8.101691
%  0.2428407  0.03093027   0.2790237   -1.535951
%  0           0           0           1
%];
%p = [
%  -1.370597  8.994232 -1.503143
%];
%np = [
%  -9.463478e-17 -7.772938e-17            -1
%];
%s = 0.371191;
%VBT = VB*model(1:3,1:3)' + model(1:3,4)';
%[VH,FH,JH] = half_space_intersect(VA,FA,p,np);
%[VT,FT,JT] = half_space_intersect(VA,FA,p,-np);
%FT = FT(JT~=0,:);
%save('spider.mat','VB','FB','VBT','VA','FA','VH','FH','JH','VT','FT','JT','s','np','p','model');
%
%msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.6,'SpecularExponent',100};
%clf;
%hold on;
%ts = {};
%ts{end+1} = tsurf(FA,[-25 10 0]+VA*axisangle2matrix([1 0 0],-pi/2),'CData',ones(size(FA,1),1),'EdgeColor','none',msoft{:},'FaceAlpha',0.9);
%ts{end+1} = tsurf(FB,VBT*axisangle2matrix([1 0 0],-pi/2),'CData',2*ones(size(VBT,1),1),'EdgeColor','none',msoft{:});
%ts{end+1} = tsurf(FH, VH*axisangle2matrix([1 0 0],-pi/2),'CData',1*(JH>0),'EdgeColor','none',msoft{:},'FaceAlpha',0.9);
%%ts{end+1} = tsurf(FT, VT*axisangle2matrix([1 0 0],-pi/2),'CData',ones(size(VT,1),1),'FaceAlpha',0.05,'EdgeColor','none',msoft{:});
%hold off;
%%apply_ambient_occlusion([ts{:}]);
%l = light('Position',1000*[-0.8 -0.8 2],'Style','infinite');
%add_shadow([ts{[1 3]}],l,'Color',0.9*[1 1 1],'Fade','infinite','Nudge',0.1);
%axis equal;
%view(-36,07);
%red = [0.8 0.2 0.2];
%green = [0.2 0.7 0.2];
%colormap([red;0.5 0.5 0.4;green;])
%camproj('persp');
%camlight;
%set(gca,'pos',[0 0 1 1])
%set(gca,'Visible','off');set(gcf,'Color','w');

tic;
% Create mesh of outer shell of transformed B
[VO,FO] = signed_distance_isosurface( ...
  VBT,FB,'Level',1,'ContouringMethod','marching_cubes', ...
  'SignedDistanceType','winding_number');
toc;

tic;
[VL,FL,JL,VR,FR,JR] = matryoshka(VA,FA,VBT,FB,p,np);
toc

save('spider.mat','VB','FB','VBT','VA','FA','VH','FH','JH','VT','FT','JT','s','np','p','model', ...
  'VL','FL','JL','VR','FR','JR');

%vidObj = VideoWriter('nefertiti.mp4','MPEG-4');
%vidObj.Quality = 100;
%vidObj.open;
matryoshka_video( ...
  VL, FL, JL, ...
  VR, FR, JR, ...
  model, np,  ...
  'AL',AL, ...
  'AR',AR, ...
  ... 'Callback',@() figgif('nefertiti.gif'), ...
  ... 'ColorMap',CM, ...
  ... 'SliceColor',SM, ...
  ... 'Callback',@() vidObj.writeVideo(myaa('raw')), ...
  'FramesPerLevel',60, ...
  'NumFaces',size(FA,1), ...
  'LightParams', {'Position',[1 -1 10],'Style','infinite'}, ...
  'Rotation',axisangle2matrix([0 0 1],-pi*0.25), ...
  'View',[15 20], ...
  'ViewChange',[0 -10]);

%vidObj.writeVideo(myaa('raw'));
%vidObj.close;
