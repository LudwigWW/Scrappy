from django.db import models
import datetime
from django.utils import timezone


class Question(models.Model):
    question_text = models.CharField(max_length=200)
    pub_date = models.DateTimeField('date published')

    def __str__(self):
        return self.question_text

    def was_published_recently(self):
        return self.pub_date >= timezone.now() - datetime.timedelta(days=1)

class Choice(models.Model):
    question = models.ForeignKey(Question, on_delete=models.CASCADE)
    choice_text = models.CharField(max_length=200)
    votes = models.IntegerField(default=0)

    def __str__(self):
        return self.choice_text

class ScrapType(models.Model):
    class_name = models.CharField(max_length=200)
    ui_name = models.CharField(max_length=200, default='')
    stl_count = models.IntegerField(default=1)
    
    scrap_name_holder = models.CharField(max_length=200, blank=True, null=True)
    scrap_description_holder = models.CharField(max_length=5000, blank=True, null=True)

    scrap_image_holder = models.FileField(upload_to='userimages/', default='', blank=True, null=True)

    id_initials = models.CharField(max_length=20, default='')

    def scrap_image_url(self):
        if self.scrap_image_holder and hasattr(self.scrap_image_holder, 'url'):
            return self.scrap_image_holder.url

    def __str__(self):
        return self.class_name  


class CustomizerClass(ScrapType):
    camera_x = models.DecimalField(max_digits=7, decimal_places=2, default=0.0)
    camera_y = models.DecimalField(max_digits=7, decimal_places=2, default=0.0)
    camera_z = models.DecimalField(max_digits=7, decimal_places=2, default=0.0)
    parameter_file = models.FileField(upload_to='parameters/', default='', blank=True, null=True)
    script_file = models.FileField(upload_to='scripts/', default='', blank=True, null=True)
    preview_counter = models.IntegerField(default=0)

    def __str__(self):
        return self.class_name  

class CustomizerInput(models.Model):
    input_description = models.CharField(max_length=200)
    input_name = models.CharField(max_length=200, unique=True, default='') # == openscad parameter name
    
    class Meta:
        abstract = True

    def __str__(self):
        return self.input_name

class Slider(CustomizerInput):
    parent_class = models.ForeignKey(CustomizerClass, related_name='sliders', on_delete=models.CASCADE)
    min_int = models.DecimalField(max_digits=3, decimal_places=1, default=0)
    max_int = models.IntegerField(default=200)
    step_size = models.DecimalField(max_digits=2, decimal_places=1, default=0.1)
    value = models.DecimalField(max_digits=7, decimal_places=1, default=50.0)
    
    def __str__(self):
        return self.input_name

class Toggle(CustomizerInput):
    parent_class = models.ForeignKey(CustomizerClass, related_name='toggles', on_delete=models.CASCADE)
    value = models.BooleanField(default=False)
    
    def __str__(self):
        return self.input_name

class ScrapObject(models.Model):
    scrap_id = models.CharField(max_length=200, unique=True)
    name_by_user = models.CharField(max_length=200, default='')
    storage_hint = models.CharField(max_length=5000, default='', blank=True)
    storage_picture = models.ImageField(upload_to='storage_pictures/', default='')
    preview_image = models.ImageField(upload_to='previews/', default='')
    stl_file = models.FileField(upload_to='STL/', default=None)

    frequency = models.IntegerField(default=0)
    frequency_text = models.CharField(max_length=20, default='Never')
    number_stored = models.IntegerField(default=0)

    def preview_image_url(self):
        if self.preview_image and hasattr(self.preview_image, 'url'):
            return self.preview_image.url

    def storage_picture_url(self):
        if self.storage_picture and hasattr(self.storage_picture, 'url'):
            return self.storage_picture.url