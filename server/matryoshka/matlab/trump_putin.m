%[VA,FA] = load_mesh('../examples/busts/trump-solid-decimated.obj');
%[VB,FB] = load_mesh('../examples/busts/putin-decimated.obj');
%p = [0.2884701 -0.2928058  0.9415544];
%np = [1 -4.371139e-08             0];
%[VH,FH,JH] = half_space_intersect(VA,FA,p,np);
%[VT,FT,JT] = half_space_intersect(VA,FA,p,-np);
%AH = ambient_occlusion(VH,FH,barycenter(VH,FH),normals(VH,FH),1000);
%AT = ambient_occlusion(VT,FT,barycenter(VT,FT),normals(VT,FT),1000);
%AB = ambient_occlusion(VB,FB,barycenter(VB,FB),normals(VB,FB),1000);

meths = {'fixed','free'};
VBT = {};
for mi = 1:2
  method = meths{mi};
  for pass = 1:4
    switch(method)
    case 'fixed'
    
    switch(pass)
    %X0<<0,0,0,0,0, 0.4,0,M_PI*0.5;
    case 1
    model = [
       0.1054688          0          0  0.2623565
               0  0.1054688          0 -0.3232672
               0          0  0.1054688  0.8945152
               0          0          0          1
    ];
    p = [
       0.2884701 -0.2928058  0.9415544
    ];
    np = [
                  1 -4.371139e-08             0
    ];
    s = 0.105469;
    %X0<<0,0,0,0,0, 0.2,0,M_PI*0.5;
    case 2
    model = [
       0.4628906          0          0  0.1738607
               0  0.4628906          0 -0.4264976
               0          0  0.4628906  0.5351045
               0          0          0          1
    ];
    p = [
       0.2884701 -0.2928058  0.7415544
    ];
    np = [
                  1 -4.371139e-08             0
    ];
    s = 0.462891;
    %X0<<0,0,0,0,0, 0.2,0,M_PI*0.5;
    case 3
    model = [
       0.5449219          0          0  0.1535501
               0  0.5449219          0 -0.4501899
               0          0  0.5449219  0.2985184
               0          0          0          1
    ];
    p = [
       0.2884701 -0.2928058  0.5415544
    ];
    np = [
                  1 -4.371139e-08             0
    ];
    s = 0.544922;
    %X0<<0,0,0,0,0,-0.2,0,M_PI*0.5;
    case 4
    model = [
       0.2995129          0          0  0.2143122
               0  0.2995129          0 -0.3793109
               0          0  0.2995129  0.2079712
               0          0          0          1
    ];
    p = [
       0.2884701 -0.2928058  0.3415544
    ];
    np = [
                  1 -4.371139e-08             0
    ];
    s = 0.299513;
    end
    case 'free'
    switch(pass)
    %X0<<0,0,0,0,0, 0.4,0,M_PI*0.5;
    case 1
    model = [
        0.2299757  0.03455889  0.01443398   0.2151104
      0.009781695  0.03125821  -0.2306918  -0.2013668
      -0.03615211   0.2282982  0.02940098   0.8714556
                0           0           0           1
    ];
    p = [
       0.2884701 -0.2928058  0.9415544
    ];
    np = [
                  1 -4.371139e-08             0
    ];
    s = 0.233005;
    case 2
    model = [
      -0.4261955  0.2613564 0.07996543  0.2828445
       0.2557052  0.3289735  0.2876379  -0.579418
      0.09652194  0.2825129 -0.4089184  0.8184389
               0          0          0          1
    ];
    p = [
       0.2884701 -0.2928058  0.7415544
    ];
    np = [
                  1 -4.371139e-08             0
    ];
    s = 0.506305;
    case 3
    model = [
      -0.09985282   0.6426681   0.0606995    0.100506
        0.6443244   0.1029745 -0.03032636  -0.4685527
      -0.03940611  0.05523836  -0.6496716   0.8251117
                0           0           0           1
    ];
    p = [
       0.2884701 -0.2928058  0.5415544
    ];
    np = [
                  1 -4.371139e-08             0
    ];
    s = 0.653205;
    case 4
    model = [
        0.0509491  -0.4815578 -0.02400255   0.4256438
       -0.4385567 -0.03625517  -0.2035275 -0.08297653
        0.2003548  0.04309881  -0.4393977   0.4754718
                0           0           0           1
    ];
    p = [
       0.2884701 -0.2928058  0.3415544
    ];
    np = [
                  1 -4.371139e-08             0
    ];
    s = 0.48484;
    end
    end
    VBT{4*(mi-1) + pass} = VB*model(1:3,1:3)'*0.995 + model(1:3,4)';
  end
end
  

msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.8,'SpecularExponent',100};
for mi = 1:2
  clf;
  hold on;
  ts = {};
  method = meths{mi};
  off = [0.7 0 0];
  for pass = 1:4
    ts{end+1} = tsurf(FB,(pass-1)*off+VBT{4*(mi-1)+pass},'FaceVertexCData',(1-AB).*blue,'EdgeColor','none',msoft{:});
                tsurf(FH,(pass-1)*off+VH,'FaceColor',orange,'EdgeColor','none',msoft{:},'FaceAlpha',0.1);
    ts{end+1} = tsurf(FT,(pass-1)*off+VT,'FaceVertexCData',(1-AT).*orange,'EdgeColor','none',msoft{:},'FaceAlpha',1.0);
  end
  hold off;
  view(34,5);
  axis equal;
    a = gca;
    cen = [mean(a.XLim) mean(a.YLim) mean(a.ZLim)];
  camlight;
  l = light('Position',[0 -0.3 1],'Style','infinite');
  add_shadow(ts,l,'Color',0.9*[1 1 1]);
  camproj('persp');
  set(gca,'pos',[0 0 1 1])
  set(gca,'Visible','off');set(gcf,'Color','w');
  drawnow;
  imwrite(getfield(getframe(gcf),'cdata'),sprintf('trump-putin-%s.png',meths{mi}));
end
