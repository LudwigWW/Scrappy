from django.contrib import admin

from .models import Question, CustomizerClass, Slider, Toggle, ScrapObject

admin.site.register(Question)
admin.site.register(CustomizerClass)
admin.site.register(Slider)
admin.site.register(Toggle)
admin.site.register(ScrapObject)
