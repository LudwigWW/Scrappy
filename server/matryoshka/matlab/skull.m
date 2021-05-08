%[VA,FA] = load_mesh('../examples/90/Skull_s.stl');
%[VA,~,I] = remove_duplicate_vertices(VA,1e-7);
%FA = I(FA);
%VA = (VA - min(VA))/max(max(VA)-min(VA));
%% Scale to be 10cm = 100mm
%VA = VA*100;
%VB = VA;
%FB = FA;
%
%model = [
%  0.901604 -0.006437491  0.007676415     3.305459
%  0.006843487    0.9003139  -0.04876663     7.659714
%  -0.007316783   0.04882189    0.9003072     3.703141
%  0            0            0            1
%];
%p = [
%  32.47979 54.22311 38.24822
%];
%np = [
%  -0.00434404   0.7321174   0.6811646
%];
%s = 0.90166;
%
%% Tolerance and wall thickness in mm
%%
%% Remember the tolerance σ is add padding on "all sides". 
%sigma = 0.7;
%epsilon = 0.5;
%
%%% Create mesh of inner shell of A
%%tic;
%%[VI,FI] = signed_distance_isosurface( ...
%%  VA,FA,'Level',-(sigma+epsilon),'ContouringMethod','cgal', ...
%%  'RadiusBound',0.010,'DistanceBound',0.010, ...
%%  'SignedDistanceType','pseudonormal');
%%%[VI,FI] = signed_distance_isosurface( ...
%%%  VA,FA,'Level',-(sigma+epsilon),'ContouringMethod','marching_cubes', ...
%%%  'GridSize',80, ...
%%%  'SignedDistanceType','pseudonormal');
%%toc
%%
%%clf;
%%hold on;
%%%tsurf(FA,VA,'FaceColor',blue,'FaceAlpha',0.5,'EdgeColor','none',fsoft);
%%tsurf(FI,VI,'FaceColor',blue,'FaceAlpha',0.8,'EdgeAlpha',0.2,fsoft);
%%%tsurf(FB,VB,'FaceColor',orange,'FaceAlpha',1.0,'EdgeColor','none',fsoft);
%%axis equal;
%%hold off;
%%camlight;
%%drawnow;
%
%%writePLY('A.ply',VI,FI);
%%writePLY('B.ply',VB,FB);
%
%% Transformed B
%VBT = VB*model(1:3,1:3)' + model(1:3,4)';
%%% CGAL is freaking out on this model and if I use marching cubes then
%%% matryoshka freaks out. This is a bit of a special case hack:
%%cenB = centroid(VBT,FB);
%%VO = VBT-cenB;
%%VO = VO*(1+sigma/100);
%%VO = VO+cenB;
%%FO = FB;
%
%tic;
%% Create mesh of outer shell of transformed B
%%[VO,FO] = signed_distance_isosurface( ...
%%  VBT,FB,'Level',sigma,'ContouringMethod','marching_cubes', ...
%%  'GridSize',45, ...
%%  'SignedDistanceType','pseudonormal');
%[VO,FO] = signed_distance_isosurface( ...
%  VBT,FB,'Level',sigma,'ContouringMethod','cgal', ...
%  'RadiusBound',0.010,'DistanceBound',0.010, ...
%  'SignedDistanceType','pseudonormal');
%toc
%
%
%clf;
%hold on;
%tsurf(FA,VA,'FaceColor',blue,'FaceAlpha',0.5,'EdgeColor','none',fsoft);
%tsurf(FI,VI,'FaceColor',blue,'FaceAlpha',0.8,'EdgeColor','none',fsoft);
%tsurf(FB,VBT,'FaceColor',orange,'FaceAlpha',1.0,'EdgeColor','none',fsoft);
%tsurf(FO,VO,'FaceColor',orange,'FaceAlpha',0.2,'EdgeColor','none',fsoft);
%quiver3(p(1),p(2),p(3),np(1),np(2),np(3),100,'Color','k','LineWidth',4);
%QV = [-1 -1 0;1 -1 0;1 1 0;-1 1 0];
%QQ = [1 2 3 4];
%[w,a] = axisanglebetween([0 0 1],np);
%R = axisangle2matrix(w,a);
%axis equal;
%axis manual;
%QV = QV*100*R'+p;
%trisurf(QQ,QV(:,1),QV(:,2),QV(:,3),'FaceColor',0.5*[1 1 1],'FaceAlpha',0.2);
%hold off;
%camlight;
%drawnow;
%
%tic;
%[VL,FL,JL,VR,FR,JR] = matryoshka(VA,FA,VO,FO,p,np);
%toc
%
%clf;
%hold on;
%tsurf(FL,VL,'FaceColor',0.5*blue,'FaceAlpha',0.5,'EdgeColor','none',fsoft);
%tsurf(FR,VR,'FaceColor',1.0*blue,'FaceAlpha',0.5,'EdgeColor','none',fsoft);
%tsurf(FB,VBT,'FaceColor',orange,'FaceAlpha',1.0,'EdgeColor','none',fsoft);
%hold off;
%axis equal;
%camlight;
%pause
%
%%writeOBJ('skull-L0.obj',VL,FL);
%%writeOBJ('skull-R0.obj',VR,FR);
%%writeOBJ('skull-L1.obj',VL*s,FL);
%%writeOBJ('skull-R1.obj',VR*s,FR);
%
%[w,a] = axisanglebetween([0 0 1],np);
%R = axisangle2matrix(w,a);
%pVL = VL*R*axisangle2matrix([1 0 0],pi);
%pVR = VR*R;
%pVL = pVL-min(pVL);
%pVR = pVR-min(pVR);
%pVR = pVR+[max(pVL(:,1))+5 0 0];
%pVB = VB*s;
%pVB = pVB*axisangle2matrix([1 0 0],-pi/2);
%pVB = pVB-min(pVB);
%pVB = pVB+[0 max(pVL(:,2))+5 0];
%
%clf;
%hold on;
%tsurf(FL,pVL,'FaceColor',0.5*blue,'FaceAlpha',0.5,'EdgeColor','none',fsoft);
%tsurf(FR,pVR,'FaceColor',1.0*blue,'FaceAlpha',0.5,'EdgeColor','none',fsoft);
%tsurf(FB,pVB,'FaceColor',1.0*orange,'FaceAlpha',0.5,'EdgeColor','none',fsoft);
%hold off;
%axis equal;
%camlight;
%
%%writeOBJ('skull-print.obj', [pVL;pVR;pVB], [FL;size(pVL,1)+[FR;size(pVR,1)+FB]]);
%
%AL = ambient_occlusion(VL,FL,barycenter(VL,FL),normals(VL,FL),1000);
%AR = ambient_occlusion(VR,FR,barycenter(VR,FR),normals(VR,FR),1000);
%AA = ambient_occlusion(VA,FA,VA,per_vertex_normals(VA,FA),1000);

%CL = 1*(JL<=size(FA,1))+1*(JL<=size(FA,1)+2);
%CR = 1*(JR<=size(FA,1))+1*(JR<=size(FA,1)+2);
%VV = [];
%FF = [];
%CC = [];
%
%MAXK = 20;
%CM = cbrewer('YlOrRd',MAXK+6);
%CM = CM(1:end-6,:);
%CM = ([CM*0.8;(1+0.7*(CM-1));CM]);
%%CM = ([CM*0.8;repmat([0 1 0],MAXK,1);CM]);
%clf;
%hold on;
%bbd = norm(max(VA)-min(VA));
%T = 0.5*bbd*np;
%R = fit_rotation(model(1:3,1:3)');
%msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.05,'AmbientStrength',0.7,'SpecularExponent',100};
%for k = 1:MAXK
%  if k < MAXK
%    Tk =  (1/(k)) * T*((R)^(k-1))';
%  else
%    Tk = [0 0 0];
%  end
%  spread = 40;
%  if k == MAXK
%    CAI = 2*ones(size(VA,1),1)*MAXK+k;
%    tsurf(FA,(([VA ones(size(VA,1),1)]*(model^(k-1))')*eye(4,3)),'FaceVertexCData',(1-AA).*squeeze(ind2rgb(CAI,CM)),'FaceAlpha',1.0,'EdgeColor','none',msoft{:});
%  else
%    CRI = CR*MAXK+k;
%    CLI = CL*MAXK+k;
%    tsurf(FR,(([VR+spread*np ones(size(VR,1),1)]*(model^(k-1))')*eye(4,3)),'FaceVertexCData',(1-AR).*squeeze(ind2rgb(CRI,CM)),'FaceAlpha',1.0,'EdgeColor','none',msoft{:});
%    tsurf(FL,(([VL-spread*np ones(size(VL,1),1)]*(model^(k-1))')*eye(4,3)),'FaceVertexCData',(1-AL).*squeeze(ind2rgb(CLI,CM)),'FaceAlpha',1.0,'EdgeColor','none',msoft{:});
%  end
%end
%error

%%  FF = [FF;size(VV,1)+FR];
%%  VV = [VV;(([VR+spread*np ones(size(VR,1),1)]*(model^(k-1))')*eye(4,3))];
%%  CC = [CC;CR*MAXK+k];
%%  FF = [FF;size(VV,1)+FL];
%%  VV = [VV;(([VL-spread*np ones(size(VL,1),1)]*(model^(k-1))')*eye(4,3))];
%%  CC = [CC;CL*MAXK+k];
%%end
%%tsurf(FF,VV,'CData',CC,'FaceAlpha',1.0,'EdgeColor','none',fsoft);
%
%l = light('Position',[-0.2 -0.5 1]*bbd*50,'Style','local');
%ss = add_shadow([],l,'Color',0.95*[1 1 1]);
%hold off;
%view(28,10);
%camlight;
%camproj('persp');
%axis equal;
%set(gca,'pos',[0 0 1 1])
%set(gca,'Visible','off');set(gcf,'Color','w');
%%colormap([zeros(MAXK,3);parula(MAXK);parula(MAXK)]);
%silver = repmat([0.8 0.8 0.8],MAXK,1);
%%CM = parula(MAXK);
%drawnow;

load('skull.mat');
MAXK = 20;
CM = cbrewer('YlOrRd',MAXK+6);
CM = CM(1:end-6,:);
CM = ([CM*0.8;(1+0.7*(CM-1));CM]);

matryoshka_video( ...
  VL, FL, JL, ...
  VR, FR, JR, ...
  model, np,  ...
  'AL',AL, ...
  'AR',AR, ...
  ... 'Callback',@() figgif('skull.gif'), ...
  ... 'Callback',@() vidObj.writeVideo(myaa('raw')), ...
  'ColorMap',CM, ...
  'Levels',MAXK, ...
  'FramesPerLevel',15, ...
  'NumFaces',size(FA,1), ...
  'OffsetFunction',@(k) 0.5*0.25*1/k, ...
  'Rotation',eye(3), ...
  'Spread',40, ...
  'View',[28,10]);
