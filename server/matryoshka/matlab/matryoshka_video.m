function matryoshka_video( ...
  VL,FL,JL,VR,FR,JR,model,np, ...
  varargin)
  % MATRYOSHKA_VIDEO
  %
  % matryoshka_video(VL,FL,JL,VR,FR,JR,model,np,'ParameterName',ParameterValue, ...)
  %
  % Inputs:
  %   VL
  %   FL
  %   JL
  %   VR
  %   FR
  %   JR
  %   model
  %   np
  %   Optional:
  %     'Callback'  followed by callback function to be called after each frame
  %       is drawn {@() drawnow}.
  %     'ColorMap'  followed by #Levels list of colors {cbrewer('Set1')}
  %     'FramesPerLevel'  followed by number of animation frames per level {5}
  %     'Levels'  followed by number of levels 6 or size of inputs
  %     'NumFaces'  followed by number of faces in original mesh, used for
  %       coloring {inf}
  %    
  %

  % Default values
  callback = @() drawnow;
  spread = 0;
  RR = eye(3);
  frames_per_level = 15;
  light_params = [];
  MAXK = [];
  CM = [];
  num_faces = [];
  slice_color = [0.6 0.6 0.6];
  offset_func = @(k) 0.4*(1/sqrt(k));
  AL = [];
  AR = [];
  VW = [15 4];
  VX = [20 0];
  bg_color = [1 1 1];
  shadow_color = 0.95*[1 1 1];
  tsurf_params = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.8,'SpecularExponent',100};
  %tsurf_params = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.8,'SpecularExponent',100,'FaceAlpha',0.1,'Visible','on'};
  % Map of parameter names to variable names
  params_to_variables = containers.Map( ...
    {'AL','AR','BackgroundColor','Callback','ColorMap','FramesPerLevel','Levels','LightParams','NumFaces','OffsetFunction','Rotation','ShadowColor','SliceColor','Spread','View','ViewChange'}, ...
    {'AL','AR','bg_color','callback','CM','frames_per_level','MAXK','light_params','num_faces','offset_func','RR','shadow_color','slice_color','spread','VW','VX'});
  v = 1;
  while v <= numel(varargin)
    param_name = varargin{v};
    if isKey(params_to_variables,param_name)
      assert(v+1<=numel(varargin));
      v = v+1;
      % Trick: use feval on anonymous function to use assignin to this workspace
      feval(@()assignin('caller',params_to_variables(param_name),varargin{v}));
    else
      error('Unsupported parameter: %s',varargin{v});
    end
    v=v+1;
  end

  if iscell(VL)
    MAXK = numel(VL);
    assert(numel(VR) == MAXK);
    assert(numel(model) == MAXK-1);
  else
    VL    = {VL};
    FL    = {FL};
    JL    = {JL};
    VR    = {VR};
    FR    = {FR};
    JR    = {JR};
    model = {model};
    np    = {np};
    if ~isempty(AL)
      AL    = {AL};
      AR    = {AR};
    end
  end

  % pad
  np = {np{:},[0 0 0]};
  if isempty(MAXK)
    MAXK = 6;
  end
  if isempty(CM)
    CM = (cbrewer('Set1',MAXK+2));
    CM = CM(2:end,:);
  end
  if isempty(num_faces)
    num_faces = inf(MAXK,1);
  end

  if isempty(AL)
    for k = 1:numel(VL)
      AL{k} = ambient_occlusion(VL{k},FL{k},barycenter(VL{k},FL{k}),normals(VL{k},FL{k}),1000);
      if isempty(FR{k})
        AR{k} = [];
      else
        AR{k} = ambient_occlusion(VR{k},FR{k},barycenter(VR{k},FR{k}),normals(VR{k},FR{k}),1000);
      end
    end
    %save('turducken.mat','-append','AL','AR');
  end

  bbd = norm(max([VL{1};VR{1}])-min([VL{1};VR{1}]));

  if isempty(light_params)
    light_params = {'Position',[0.4 -0.8 1]*bbd*50,'Style','local'};
  end

  function T = tform(k)
    if k == 1
      T = eye(4);
    else
      T = tform(k-1)*model{min(k-1,end)};
    end
  end

  tr = {};
  tl = {};
  clf;
  hold on;
  for k = 1:MAXK
    kk = min(k,numel(VL));
    CL = (JL{kk}>num_faces(kk)).*slice_color(min(k,end),:) + (JL{kk}<=num_faces(kk)).*CM(k,:);
    tl{k} =   tsurf(FL{kk},(([VL{kk} ones(size(VL{kk},1),1)]*tform(k)')*eye(4,3))*RR,'FaceVertexCData',(1-AL{kk}).*CL,'FaceAlpha',1.0,'EdgeColor','none','Visible','off',tsurf_params{:});
    if ~isempty(FR{kk})
      CR = (JR{kk}>num_faces(kk)).*slice_color(min(k,end),:) + (JR{kk}<=num_faces(kk)).*CM(k,:);
      tr{k} = tsurf(FR{kk},(([VR{kk} ones(size(VR{kk},1),1)]*tform(k)')*eye(4,3))*RR,'FaceVertexCData',(1-AR{kk}).*CR,'FaceAlpha',1.0,'EdgeColor','none','Visible','off',tsurf_params{:});
    end                                                                                                                                                                
  end
  hold off;
  view(25,4);
  camlight;
  camproj('persp');
  axis equal;
  set(gca,'pos',[0 0 1 1])
  set(gca,'Visible','off');
  set(gcf,'Color',bg_color);
  l = light(light_params{:});
  view(35,4);
  cen = 0.5*(max([VL{1};VR{1}])+min([VL{1};VR{1}]))*RR;

  ease = @(X) (X<0).*0 + (X>1).*1 + (X>=0 & X<=1).*(3*X.^2-2*X.^3);
  first = true;
  for K = MAXK-1:-1:1
    for w = linspace(1,0,frames_per_level)
      s = ease(w);
      for k = 1:MAXK
        kk = min(k,numel(VL));
        tl{k}.Visible = 'off';
        if ~isempty(FR{kk})
          tr{k}.Visible = 'off';
        end
      end
      for k = min(K+1,MAXK):-1:1
        kk = min(k,numel(VL));
        T = bbd*np{kk};
        R = fit_rotation( (eye(3,4)*tform(k)*eye(4,3))' )';
        Tk =  offset_func(k) * T*R;
        if k == K
          Tk = s*Tk;
        elseif k<K
          Tk = 1*Tk;
        else
          Tk = [0 0 0];
        end
        tl{k}.Vertices = (([VL{kk}-((k<K)*1+(k==K)*s)*(k<MAXK)*spread*np{kk} ones(size(VL{kk},1),1)]*tform(k)')*eye(4,3)-Tk)*RR;
        if ~isempty(FR{kk})
          tr{k}.Vertices = (([VR{kk}+((k<K)*1+(k==K)*s)*(k<MAXK)*spread*np{kk} ones(size(VR{kk},1),1)]*tform(k)')*eye(4,3)+Tk)*RR;
        end
        if k <= K || s>0
          if ~isempty(FR{kk})
            tr{k}.Visible = 'on';
          end
          tl{k}.Visible = 'on';
        end
      end
      v = (w*(frames_per_level/(frames_per_level+1))+K-1)/(MAXK);
      view(VW(1)+VX(1)*(1-v),VW(2)+VX(2)*(1-v));
      if first
        g = axis*[0;0;0;0;1;0]-14.71*bbd/160.72;
      end
      ss = add_shadow( ...
        [tr{1:min(K+1,end)} tl{1:K+1}], ...
        l, ...
        'Ground',[0 0 -1 g], ...
        'Color',shadow_color, ...
        'BackgroundColor',bg_color);
      if first
        axis(reshape([-1;1]*max( abs(reshape(axis',2,[])-cen))+cen,[],1)');
        axis manual;
        axis vis3d;
        first = false;
      end
      callback();
      delete(ss);
    end
  end

end
