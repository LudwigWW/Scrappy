[VA,FA] = load_mesh('../examples/60/menouFinal2.stl');
[VA,~,I] = remove_duplicate_vertices(VA,1e-7);
FA = I(FA);
VB = VA;
FB = FA;
VT = {};
FT = {};
VH = {};
FH = {};
JH = {};
VBT = {};
for pass = 1:3
  switch pass
  case 1
    % Fixed rotation and fixed position
    model = [
      0.3980494         0         0  8.361712
              0 0.3980494         0  8.173437
              0         0 0.3980494  15.91559
              0         0         0         1
    ];
    p = [
      13.89102 13.57825 26.44003
    ];
    np = [
      0.08987855  0.9959527          0
    ];
    s = 0.398049;
  case 2  
    % Fixed rotation and free position
    model = [
      0.5328351         0         0 -2.116612
              0 0.5328351         0  5.964693
              0         0 0.5328351  9.654667
              0         0         0         1
    ];
    p = [
      5.285013 13.19966 23.74284
    ];
    np = [
      0.08987855  0.9959527          0
    ];
    s = 0.532835;
  case 3
    % Free rotation and free position
    model = [
       -0.4819123   0.2329965   0.3314966     5.01094
      0.004373829  -0.5120869    0.366285     11.8484
        0.4051645   0.2826596   0.3903357    6.335099
                0           0           0           1
    ];
    p = [
      10.24515  14.6405 26.12176
    ];
    np = [
      0.08987855  0.9959527          0
    ];
    s = 0.629616;
  end
  VBT{pass} = VB*model(1:3,1:3)' + model(1:3,4)';
  [VH{pass},FH{pass},JH{pass}] = half_space_intersect(VA,FA,p,np);
  [VT{pass},FT{pass},JT] = half_space_intersect(VA,FA,p,-np);
  FT{pass} = FT{pass}(JT~=0,:);
end

msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.6,'SpecularExponent',100};
clf;
hold on;
off = [60 -50 0];
ts = {};
for pass = 1:3
  ts{end+1} = tsurf(      FB,(pass-1)*off+VBT{pass},'CData',2*ones(size(VBT,1),1),'EdgeColor','none',msoft{:});
  ts{end+1} = tsurf(FH{pass},(pass-1)*off+VH{pass},'CData',1*(JH{pass}>0),'EdgeColor','none',msoft{:},'FaceAlpha',0.9);
  tsurf(FT{pass},(pass-1)*off+VT{pass},'CData',ones(size(VT{pass},1),1),'FaceAlpha',0.1,'EdgeColor','none',msoft{:});
end
hold off;
l = light('Position',1000*[-0.5 -0.2 1],'Style','infinite');
add_shadow(ts,l,'Color',0.9*[1 1 1],'Fade','infinite');
axis equal;
view(-45,27);
colormap([1+0.2*(blue-1);blue;orange;])
camproj('persp');
camlight;
set(gca,'pos',[0 0 1 1])
set(gca,'Visible','off');set(gcf,'Color','w');
