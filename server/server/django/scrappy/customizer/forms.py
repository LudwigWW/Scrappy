from django import forms

from .models import CustomizerClass, ScrapObject

class PostForm(forms.ModelForm):
    scrap_image_holder = forms.ImageField(label='')
    class Meta:
        model = CustomizerClass
        fields = ('scrap_image_holder', )

class PreviewForm(forms.ModelForm):
    preview_image = forms.ImageField(label='Scrap image')
    class Meta:
        model = ScrapObject
        fields = ('preview_image', )

class StorageForm(forms.ModelForm):
    storage_picture = forms.ImageField(label='Storage picture')
    class Meta:
        model = ScrapObject
        fields = ('storage_picture', )
