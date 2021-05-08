%[VA,FA] = load_mesh('../examples/60/bunny.ply');
%VA = (VA - min(VA))/max(max(VA)-min(VA));
%% Scale to be 10cm = 100mm
%VA = VA*100;
%VB = VA;
%FB = FA;
%
%% Tolerance and wall thickness in mm
%%
%% Remember the tolerance σ is add padding on "all sides". 
%sigma = 0.5;
%epsilon = 0.1;
%
%% Create mesh of inner shell of A
%tic;
%[VI,FI] = signed_distance_isosurface( ...
%  VA,FA,'Level',-(sigma+epsilon),'ContouringMethod','cgal', ...
%  'RadiusBound',0.010,'DistanceBound',0.010, ...
%  'SignedDistanceType','pseudonormal');
%toc
%
%clf;
%hold on;
%%tsurf(FA,VA,'FaceColor',blue,'FaceAlpha',0.5,'EdgeColor','none',fsoft);
%tsurf(FI,VI,'FaceColor',blue,'FaceAlpha',0.8,'EdgeAlpha',0.2,fsoft);
%tsurf(FB,VB,'FaceColor',orange,'FaceAlpha',1.0,'EdgeColor','none',fsoft);
%axis equal;
%hold off;
%camlight;
%drawnow;
%
%writePLY('A.ply',VI,FI);
%writePLY('B.ply',VB,FB);
%pause
%model = [
%  0.4115475 -0.4211284  0.1393118   44.78821
%  0.4077308  0.2843153 -0.3450342   19.00959
%  0.1746781  0.3285482  0.4771497   2.122298
%  0          0          0          1
%];
%p = [
%  56.16737 32.07476 44.09407
%];
%np = [
%  0 1 0
%];
%s = 0.605085;
%
%% Transformed B
%VBT = VB*model(1:3,1:3)' + model(1:3,4)';
%
%tic;
%% Create mesh of outer shell of transformed B
%[VO,FO] = signed_distance_isosurface( ...
%  VBT,FB,'Level',sigma,'ContouringMethod','marching_cubes', ...
%  'SignedDistanceType','winding_number');
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
%writeOBJ('print.obj', [pVL;pVR;pVB], [FL;size(pVL,1)+[FR;size(pVR,1)+FB]]);
%
%
%
%AL = ambient_occlusion(VL,FL,barycenter(VL,FL),normals(VL,FL),1000);
%AR = ambient_occlusion(VR,FR,barycenter(VR,FR),normals(VR,FR),1000);
%AR((AR>0.7)&any(reshape(VR(FR,3),size(FR))>50,2)) = nan;
%save('bunny.mat','VL','FL','JL','VR','FR','JR','model','VA','FA','np','AL','AR');

load('bunny.mat')
msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.8,'SpecularExponent',100};
MAXK = 6;
%CM = (parula(MAXK));
bbd = norm(max(VA)-min(VA));
T = bbd*np;
R = fit_rotation(model(1:3,1:3)');
%clf;
%hold on;
%for K = 1:MAXK
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
%      slide = [25 0 0];
%    end
%    CL = (JL>size(FA,1)).*0.6 + (JL<=size(FA,1)).*CM(k,:);
%    CR = (JR>size(FA,1)).*0.6 + (JR<=size(FA,1)).*CM(k,:);
%    tsurf(FR,slide+(([VR ones(size(VR,1),1)]*(model^(k-1))')*eye(4,3)+Tk)*axisangle2matrix([1 0 0],-pi/2),'FaceVertexCData',(1-AR).*CR,'FaceAlpha',1.0,'EdgeColor','none',msoft{:});
%    tsurf(FL,slide+(([VL ones(size(VL,1),1)]*(model^(k-1))')*eye(4,3)-Tk)*axisangle2matrix([1 0 0],-pi/2),'FaceVertexCData',(1-AL).*CL,'FaceAlpha',1.0,'EdgeColor','none'     ,msoft{:});
%  end
%end
%l = light('Position',[0.4 -0.8 1]*bbd*50,'Style','local');
%ss = add_shadow([],l,'Ground',[0 0 -1 -79],'Color',0.95*[1 1 1]);
%hold off;
%view(25,4);
%camlight;
%camproj('persp');
%axis equal;
%set(gca,'pos',[0 0 1 1])
%set(gca,'Visible','off');set(gcf,'Color','w');
%drawnow;

%vidObj = VideoWriter('bunny.mp4','MPEG-4');
%vidObj.Quality = 100;
%vidObj.open;
matryoshka_video( ...
  VL, FL, JL, ...
  VR, FR, JR, ...
  model, np,  ...
  'AL',AL, ...
  'AR',AR, ...
  ... 'Callback',@() figgif('bunny.gif'), ...
  ... 'Callback',@() vidObj.writeVideo(myaa('raw')), ...
  'FramesPerLevel',4, ...
  'NumFaces',size(FA,1), ...
  'Rotation',axisangle2matrix([1 0 0],-pi/2));


%vidObj.writeVideo(myaa('raw'));
%vidObj.close;

