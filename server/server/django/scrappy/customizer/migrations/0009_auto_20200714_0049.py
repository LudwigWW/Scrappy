# Generated by Django 3.0.7 on 2020-07-14 04:49

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('customizer', '0008_auto_20200714_0043'),
    ]

    operations = [
        migrations.AlterField(
            model_name='slider',
            name='input_name',
            field=models.CharField(default='Height', max_length=200, unique=True),
        ),
        migrations.AlterField(
            model_name='toggle',
            name='input_name',
            field=models.CharField(default='Height', max_length=200, unique=True),
        ),
    ]
