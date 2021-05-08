function s = find_max_scale(A,B,cen,p,np,th,varargin)
  %
  % Inputs:
  %   A  #A by 2 list of vertices along A polygon in its reference frame
  %   B  #B by 2 list of vertices along B polygon in its reference frame
  %   cen  2-vector placing B's centroid in reference frame of A
  %   p  2-vector point on clipping plane
  %   np  2-vector normal of clippling plane
  %
  %

  function valid = isvalid(s)

    valid = true;
    sigs = [1 -1];
    for si = 1:2
      sig = -((si-1)*2-1); %[-1 1]
      if ~valid 
        break;
      end
      signp = sig*np;
      t = signp * [cos(th(si)) sin(th(si));-sin(th(si)) cos(th(si))];
      % "corners" where normals change sign
      C = sparse(EB,repmat(sign(sum(bsxfun(@times,NB,t),2))+2,1,2),1,size(PB,1),3)~=0;
      CI = sum(C,2)>1;
      % Vertices above clipping plane
      PAI = sum(bsxfun(@times,bsxfun(@minus,PA,p),signp),2)>0;
      % Edges (at least partially) above clipping plane
      EAA = EA(PAI(EA(:,1)) | PAI(EA(:,2)),:);
      % Vertices above clipping plane
      PBI = sum(bsxfun(@times,bsxfun(@minus,bsxfun(@plus,s*PB,cen),p),signp),2)>0;
      % Edges (at least partially) above clipping plane
      EBB = EB(PBI(EB(:,1)) | PBI(EB(:,2)),:);
      % appropriately scaled translation vector
      ts = -(max(max(A)-min(A)) + s*max(max(B)-min(B)))*t;
      valid = valid && ~(any(any(lineSegmentIntersect( ...
        [PA(EAA(:,1),:)       PA(EAA(:,2),:)], ...
          bsxfun(@plus,[ ...
          [s*PB(EBB(:,1),:) s*PB(EBB(:,2),:)]; ... % edges on B
          [s*PB(CI&PBI,:)   bsxfun(@plus,s*PB(CI&PBI,:),ts)]],[cen cen])))));
      %clf;
      %hold on;
      %plot_edges(PA,EAA,'Color',blue);
      %plot_edges(s*PB+cen,EBB,'Color',orange);
      %CC = [s*PB(CI&PBI,:)+cen;s*PB(CI&PBI,:)+cen+ts];
      %plot_edges( ...
      %  CC,[1:size(CC,1)/2;size(CC,1)/2+(1:size(CC,1)/2)]','Color',orange*0.5);
      %scatter( s*PB(CI&PBI,1)+cen(1), s*PB(CI&PBI,2)+cen(2),'or');
      %quiver(p(:,1),p(:,2), signp(:,1), signp(:,2),2,'k','LineWidth',2);
      %quiver(p(:,1),p(:,2), signp(:,2),-signp(:,1),40,'--k','LineWidth',1);
      %quiver(p(:,1),p(:,2),-signp(:,2), signp(:,1),40,'--k','LineWidth',1);
      %hold off;
      %axis equal;
      %pause;
    end
  end
  
  persistent max_s;
  %max_s = [];
  if isempty(max_s)
    max_s = 0;
  end
  min_s = 0;

  if ~inpolygon(cen(1),cen(2),A(:,1),A(:,2))
    s = 0;
    return;
  end

  assert(all(th >=-pi/2 & th<=pi/2));

  % For now assume that translation direction is the same as the normal
  % direction

  % default values
  u = 1.0;
  l = eps;
  max_diff = 1e-3;
  % Map of parameter names to variable names
  params_to_variables = containers.Map( ...
    {'MaxScale','MaxDiff','MinScale'}, ...
    {'max_s','max_diff','min_s'});
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
  if max_s>0
    s = (u+l)*0.5;
  else
    s = max_s;
  end


  % Make sure plane normal is unit
  np = normalizerow(np);

  PA = A;
  EA = [1:size(A,1);2:size(A,1) 1]';
  PB = B;
  EB = [1:size(B,1);2:size(B,1) 1]';
  % Normal
  NB = (PB(EB(:,1),:)-PB(EB(:,2),:))*[0 1;-1 0];

  if min_s > 0 && ~isvalid(min_s)
    s = 0;
    return;
  end

  warnStruct = warning('off');
  while true

    % Merely check if BB fits in A
    %BB = s*B+cen;
    %valid = isempty(intersections(A(:,1),A(:,2),BB(:,1),BB(:,2)));
    %valid = ~(any(any(lineSegmentIntersect( ...
    %  [A(1:end-2,:) A(2:end-1,:)], ...
    %  [BB(1:end-2,:) BB(2:end-1,:)]))));
    valid = isvalid(s);

    if valid
      l = s;
    else
      u = s;
    end
    if u <= max_s
      %s = -inf;
      %% This causes ga to get worse resulst
      %s = 0; 
      s = max_s-eps;
      break;
    end

    if u-l < max_diff && l > min_s
      s = l;
      break;
    end
    s = 0.5*(u + l);
  end
  warning(warnStruct);
  if s > max_s
    max_s = s;
  end
  
end
