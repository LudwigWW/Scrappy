from django.urls import path
from django.conf.urls import url
from django.conf import settings
from django.conf.urls.static import static
from django.contrib.staticfiles.urls import staticfiles_urlpatterns

from . import views

app_name = 'customizer'
urlpatterns = [
    # ex: /customizer/
    path('', views.index, name='index'),
    # ex: /customizer/5/
    path('specifics/<int:question_id>/', views.detail, name='detail'),
    # ex: /customizer/5/results/
    path('<int:question_id>/results/', views.results, name='results'),
    # ex: /customizer/5/vote/
    path('<int:question_id>/vote/', views.vote, name='vote'),
    path('<int:customizerClass_id>/', views.customizerClass, name='customizerClass'),
    path('library/', views.STL_overview, name='STL_overview'),
    path('scrapObjects/<int:scrapObject_id>/', views.scrapObjectDetail, name='scrapObject'),

    path('<int:customizerClass_id>/new/', views.post_new, name='post_new'),
    url(r'^ajax/update_customizer/$', views.update_customizer, name='update_customizer'),
] 