# Generated by Django 3.0.7 on 2020-07-13 18:36

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('customizer', '0001_initial'),
    ]

    operations = [
        migrations.AlterField(
            model_name='scraptype',
            name='scrap_description_holder',
            field=models.CharField(max_length=5000, null=True),
        ),
        migrations.AlterField(
            model_name='scraptype',
            name='scrap_name_holder',
            field=models.CharField(max_length=200, null=True),
        ),
    ]
