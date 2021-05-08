%[VA,FA] = load_mesh('../examples/busts/Nefertiti-decimated.ply');
%VB = VA;
%FB = FA;
%
%model = [
%  0.5616597   0.04009255 -0.005624323     2.789329
%  -0.02838811    0.4457913    0.3428773    -24.13367
%  0.02886452   -0.3417065    0.4466589     78.03888
%  0            0            0            1
%];
%p = [
%  1.855125 -17.10641  97.54407
%];
%np = [
%  0.6569347 -0.6449379   0.390502
%];
%s = 0.563117;
%VBT = VB*model(1:3,1:3)' + model(1:3,4)';
%
%%[VH,FH,JH] = half_space_intersect(VA,FA,p,np);
%%[VT,FT,JT] = half_space_intersect(VA,FA,p,-np);
%%FT = FT(JT~=0,:);
%
%%tic;
%%% Create mesh of outer shell of transformed B
%%[VO,FO] = signed_distance_isosurface( ...
%%  VBT,FB,'Level',1,'ContouringMethod','marching_cubes', ...
%%  'SignedDistanceType','winding_number');
%%toc;
%tic;
%[VL,FL,JL,VR,FR,JR] = matryoshka(VA,FA,VBT,FB,p,np);
%toc

%AL = ambient_occlusion(VL,FL,barycenter(VL,FL),normals(VL,FL),1000);
%AR = ambient_occlusion(VR,FR,barycenter(VR,FR),normals(VR,FR),1000);
%save('nefertiti.mat');
%%AR((AR>0.7)&any(reshape(VR(FR,3),size(FR))>50,2)) = nan;
%
%msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.8,'SpecularExponent',100};
%MAXK = 6;
%%CM = (parula(MAXK));
%CM = (cbrewer('Set1',MAXK+2));CM = CM(2:end,:);
%clf;
%hold on;
%bbd = norm(max(VA)-min(VA));
%T = bbd*np;
%R = fit_rotation(model(1:3,1:3)');
%K = MAXK
%  for k = 1:min(K+1,MAXK)
%  %
%    if k < K
%      Tk =  0.4*(1/sqrt(k)) * T*((R)^(k-1))';
%    elseif k == K && K<MAXK
%      Tk =  0.1*(1/(k)) * T*((R)^(k-1))';
%    else
%      Tk = [0 0 0];
%    end
%    if K > 1
%      slide = (K-1)*[180 0 0];
%    else
%      slide = [-225 0 0];
%    end
%    CL = (JL>size(FA,1)).*0.6 + (JL<=size(FA,1)).*CM(k,:);
%    CR = (JR>size(FA,1)).*0.6 + (JR<=size(FA,1)).*CM(k,:);
%    tsurf(FR,slide+(([VR ones(size(VR,1),1)]*(model^(k-1))')*eye(4,3)+Tk),'FaceVertexCData',(1-AR).*CR,'FaceAlpha',1.0,'EdgeColor','none',msoft{:});
%    tsurf(FL,slide+(([VL ones(size(VL,1),1)]*(model^(k-1))')*eye(4,3)-Tk),'FaceVertexCData',(1-AL).*CL,'FaceAlpha',1.0,'EdgeColor','none'     ,msoft{:});
%  end
%
%
%
%l = light('Position',[0.4 -0.8 1]*bbd*50,'Style','local');
%ss = add_shadow([],l,'Color',0.95*[1 1 1]);
%hold off;
%view(-24,10);
%camlight;
%camproj('persp');
%axis equal;
%set(gca,'pos',[0 0 1 1])
%set(gca,'Visible','off');set(gcf,'Color','w');
%drawnow;

load('nefertiti.mat')
%CM = [213 190 166;199 171 150; 149 126 112; 118 99 87; 72 66 62]/255;
%CM = interp1(linspace(1,0,size(CM,1)),CM,linspace(0,1,6),'pchip');
%SM = hsv2rgb(rgb2hsv(CM*diag([1 0.9 0.9]))*diag([1 0.1 1.1]));
%%SM = [0.7 0.6 0];

vidObj = VideoWriter('nefertiti.mp4','MPEG-4');
vidObj.Quality = 100;
vidObj.open;
matryoshka_video( ...
  VL, FL, JL, ...
  VR, FR, JR, ...
  model, np,  ...
  'AL',AL, ...
  'AR',AR, ...
  ... 'Callback',@() figgif('nefertiti.gif'), ...
  ... 'ColorMap',CM, ...
  ... 'SliceColor',SM, ...
  'Callback',@() vidObj.writeVideo(myaa('raw')), ...
  'FramesPerLevel',60, ...
  'NumFaces',size(FA,1), ...
  'LightParams', {'Position',[1 -1 10],'Style','infinite'}, ...
  'Rotation',axisangle2matrix([0 0 1],-pi*0.25), ...
  'View',[15 20], ...
  'ViewChange',[0 -10]);

vidObj.writeVideo(myaa('raw'));
vidObj.close;
