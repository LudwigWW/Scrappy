%%[VA,FA] = load_mesh('../examples/60/COLONEL_72k_v2.stl');
%%[VA,~,I] = remove_duplicate_vertices(VA,1e-7);
%%FA = I(FA);
%%VB = VA;
%%FB = FA;
%%VL = {};
%%FL = {};
%%JL = {};
%%VR = {};
%%FR = {};
%%JR = {};
%%VBT = {};
%%np = {};
%%for pass = 1:3
%%  switch pass
%%  case 1
%%  model = [
%%    0.624482 -0.08790451 -0.02104616    28.56648
%%    -0.08456153  -0.5162659  -0.3527985    118.7809
%%    0.03192946   0.3519804  -0.5227219    95.45589
%%    0           0           0           1
%%  ];
%%  p = [
%%    50.12374 59.13293 83.28964
%%  ];
%%  np{pass} = [
%%    -8.321052e-17  8.985326e-17            -1
%%  ];
%%  s = 0.63099;
%%  case 2
%%    model = [
%%      0.599641   0.008405692      0.072529      17.43686
%%      -0.008466938     0.6040106 -5.246095e-08      28.73365
%%      -0.07252189   -0.00101655        0.5997      59.33667
%%      0             0             0             1
%%    ];
%%    p = [
%%      50.15233 65.69357 96.56585
%%    ];
%%    np{pass} = [
%%      -5.552511e-17 -1.091538e-16            -1
%%    ];
%%  case 3
%%    model = [
%%      0.5645412   -0.06722294  0.0001887325      24.55151
%%      0.06722294     0.5645412 -4.970223e-08      33.17954
%%      -0.0001874026  2.236509e-05     0.5685293      54.59456
%%      0             0             0             1
%%    ];
%%    p = [
%%      50 66 80
%%    ];
%%    np{pass} = [
%%        0.25 -1.4 -1
%%      ];
%%    s = 0.56;
%%  end
%%  VBT{pass} = VB*model(1:3,1:3)' + model(1:3,4)';
%%  [VO,FO] = signed_distance_isosurface( ...
%%    VBT{pass},FB,'Level',-0.1,'ContouringMethod','cgal', ...
%%    'RadiusBound',0.010,'DistanceBound',0.010, ...
%%    'SignedDistanceType','pseudonormal');
%%  [VL{pass},FL{pass},JL{pass},VR{pass},FR{pass},JR{pass}] = ...
%%    matryoshka(VA,FA,VO,FO,p,np{pass});
%%end
%
%bbd = norm(max(VA)-min(VA));
%msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.6,'SpecularExponent',100};
%off = [0 0 0];
%
%CM = cbrewer('RdYlGn',9);
%gold = CM(2+(1:3),:);
%green = CM(end-2:end,:);
%clf;
%hold on;
%for pass = 1:3
%  VRp = VR{pass}+0.125*bbd*normalizerow(np{pass});
%  VLp = VL{pass}-0.125*bbd*normalizerow(np{pass});
%  VBTp = VBT{pass};
%  x = [0.5*(max(VRp(:,1:2))+min(VRp(:,1:2)))+[0 -150] min(VRp(:,3))];
%  VRp = VRp -x;
%  VLp = VLp -x;
%  VBTp =VBTp-x;
%  R = axisangle2matrix([0 0 1],(pass-2)*pi*0.3);
%  VRp = VRp *R;
%  VLp = VLp *R;
%  VBTp =VBTp*R;
%
%  CLp = 1*(JL{pass}<=size(FA,1))+1*(JL{pass}<=size(FA,1)+2);
%  CRp = 1*(JR{pass}<=size(FA,1))+1*(JR{pass}<=size(FA,1)+2);
%  tsurf(FR{pass},pass*off+VRp, 'FaceVertexCData',(CRp~=2).*[0.8 0.8 0.8]+(CRp==2).*green(pass,:),'FaceAlpha',1.0,'EdgeColor','none',msoft{:});
%  tsurf(FL{pass},pass*off+VLp, 'FaceVertexCData',(CLp~=2).*[0.8 0.8 0.8]+(CLp==2).*green(pass,:),'FaceAlpha',1.0,'EdgeColor','none',msoft{:});
%  tsurf(FB      ,pass*off+VBTp,'FaceColor',gold(pass,:),'FaceAlpha',1.0,'EdgeColor','none',msoft{:});
%end
%apply_ambient_occlusion();
%l = light('Position',[0 0 800],'Style','local');
%ss = add_shadow([],l,'Color',0.9*[1 1 1]);
%hold off;
%view(0,10);
%drawnow;
%camlight;
%camproj('persp');
%axis equal;
%set(gca,'pos',[0 0 1 1])
%set(gca,'Visible','off');
%set(gcf,'Color','w');
%drawnow;
view(0,10);
drawnow;
frame = getframe(gcf);
imwrite(frame.cdata,'colonels-front.png');
view(-180,10);
drawnow;
frame = getframe(gcf);
imwrite(frame.cdata,'colonels-back.png');
